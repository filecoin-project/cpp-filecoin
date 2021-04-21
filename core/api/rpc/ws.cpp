/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/ws.hpp"

#include <queue>

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "api/rpc/json.hpp"
#include "api/rpc/make.hpp"
#include "codec/json/json.hpp"
#include "common/logger.hpp"

namespace fc::api {
  namespace beast = boost::beast;
  namespace websocket = beast::websocket;
  namespace net = boost::asio;
  using rpc::OkCb;

  common::Logger logger = common::createLogger("sector server");

  constexpr auto kParseError = INT64_C(-32700);
  constexpr auto kInvalidRequest = INT64_C(-32600);
  constexpr auto kMethodNotFound = INT64_C(-32601);

  const auto kChanCloseDelay{boost::posix_time::milliseconds(100)};

  struct SocketSession : std::enable_shared_from_this<SocketSession> {
    SocketSession(tcp::socket &&socket, const Rpc &api_rpc)
        : socket{std::move(socket)},
          timer{this->socket.get_executor()},
          rpc(std::move(api_rpc)) {}

    template <class Body, class Allocator>
    void doAccept(http::request<Body, http::basic_fields<Allocator>> req) {
      socket.async_accept(req, [self{shared_from_this()}](auto ec) {
        if (ec) {
          return;
        }
        self->doRead();
      });
    }

    void doRead() {
      socket.async_read(buffer, [self{shared_from_this()}](auto ec, auto) {
        if (ec) {
          return;
        }
        self->onRead();
        self->doRead();
      });
    }

    void onRead() {
      std::string_view s_req{static_cast<const char *>(buffer.cdata().data()),
                             buffer.cdata().size()};
      auto j_req{codec::json::parse(s_req)};
      buffer.clear();
      if (!j_req) {
        return _write(Response{{}, Response::Error{kParseError, "Parse error"}},
                      {});
      }
      auto maybe_req = decode<Request>(*j_req);
      if (!maybe_req) {
        return _write(
            Response{{}, Response::Error{kInvalidRequest, "Invalid request"}},
            {});
      }
      auto &req = maybe_req.value();
      auto respond = [id{req.id}, self{shared_from_this()}](auto res) {
        if (id) {
          self->_write(Response{*id, std::move(res)}, {});
        }
      };
      auto it = rpc.ms.find(req.method);
      if (it == rpc.ms.end() || !it->second) {
        return respond(Response::Error{kMethodNotFound, "Method not found"});
      }
      it->second(
          req.params,
          std::move(respond),
          [&]() { return next_channel++; },
          [self{shared_from_this()}](auto method, auto params, auto cb) {
            Request req{self->next_request++, method, std::move(params)};
            if (method == "xrpc.ch.close") {
              self->timer.expires_from_now(kChanCloseDelay);
              self->timer.async_wait(
                  [self, req{std::move(req)}, cb{std::move(cb)}](auto) {
                    self->_write(req, std::move(cb));
                  });
              return;
            }
            self->_write(req, std::move(cb));
          });
    }

    template <typename T>
    void _write(const T &v, OkCb cb) {
      pending_writes.emplace(*codec::json::format(encode(v)), std::move(cb));
      _flush();
    }

    void _flush() {
      if (!writing && !pending_writes.empty()) {
        auto &[buffer, cb] = pending_writes.front();
        writing = true;
        socket.async_write(
            net::buffer(buffer.data(), buffer.size()),
            [self{shared_from_this()}, cb{std::move(cb)}](auto e, auto) {
              self->writing = false;
              auto ok = !e;
              if (!ok) {
                self->pending_writes = {};
              } else {
                self->pending_writes.pop();
                self->_flush();
              }
              if (cb) {
                cb(ok);
              }
            });
      }
    }

    std::queue<std::pair<Buffer, OkCb>> pending_writes;
    bool writing{false};
    uint64_t next_channel{}, next_request{};
    websocket::stream<tcp::socket> socket;
    net::deadline_timer timer;
    beast::flat_buffer buffer;
    Rpc rpc;
  };

  struct HttpSession : public std::enable_shared_from_this<HttpSession> {
    HttpSession(tcp::socket &&socket,
                std::shared_ptr<Rpc> rpc,
                std::shared_ptr<Routes> routes)
        : stream(std::move(socket)),
          routes(std::move(routes)),
          rpc{std::move(rpc)} {}

    void run() {
      net::dispatch(stream.get_executor(),
                    [self{shared_from_this()}]() { self->doRead(); });
    }

    void doRead() {
      request = {};

      http::async_read(
          stream,
          buffer,
          request,
          [self{shared_from_this()}](beast::error_code ec,
                                     std::size_t bytes_transferred) {
            self->onRead(ec);
          });
    }

    void onRead(boost::system::error_code ec) {
      if (ec == http::error::end_of_stream) return doClose();

      if (ec) {
        logger->error(ec.message());
        return;
      }

      if (websocket::is_upgrade(request)) {
        std::make_shared<SocketSession>(stream.release_socket(), *rpc)
            ->doAccept(std::move(request));
        return;
      }

      handleRequest();
    }

    void handleRequest() {
      bool is_handled = false;
      for (auto &route : *routes) {
        if (request.target().starts_with(route.first)) {
          w_response = route.second(request);
          is_handled = true;
          break;
        }
      }
      if (not is_handled) {
        http::response<http::empty_body> response;
        response.version(request.version());
        response.keep_alive(false);
        response.result(http::status::bad_request);
        w_response.response = std::move(response);
      }
      doWrite();
    }

    // aka visitor
    void doWrite() {
      if (auto d_response = std::get_if<http::response<http::string_body>>(
              &(w_response.response))) {
        doWrite(*d_response);
      } else if (auto f_response = std::get_if<http::response<http::file_body>>(
                     &(w_response.response))) {
        doWrite(*f_response);
      } else if (auto e_response =
                     std::get_if<http::response<http::empty_body>>(
                         &(w_response.response))) {
        doWrite(*e_response);
      }
    }

    template <typename T>
    void doWrite(http::response<T> &response) {
      response.content_length(response.body().size());

      http::async_write(
          stream,
          response,
          [self{shared_from_this()}](boost::beast::error_code ec, std::size_t) {
            self->stream.socket().shutdown(tcp::socket::shutdown_send, ec);
          });
    }

    void doWrite(http::response<http::empty_body> &response) {
      http::async_write(
          stream,
          response,
          [self{shared_from_this()}](boost::beast::error_code ec, std::size_t) {
            self->stream.socket().shutdown(tcp::socket::shutdown_send, ec);
          });
    }

    void doClose() {
      boost::system::error_code ec;
      stream.socket().shutdown(tcp::socket::shutdown_send, ec);
    }

    // TODO: maybe add queue for requests

    beast::tcp_stream stream;
    beast::flat_buffer buffer;
    http::request<http::dynamic_body> request;
    WrapperResponse w_response;
    std::shared_ptr<Routes> routes;
    std::shared_ptr<Rpc> rpc;
  };

  struct Server : std::enable_shared_from_this<Server> {
    Server(tcp::acceptor &&acceptor,
           std::shared_ptr<Rpc> rpc,
           std::shared_ptr<Routes> routes)
        : acceptor{std::move(acceptor)},
          rpc{std::move(rpc)},
          routes{std::move(routes)} {}

    void run() {
      doAccept();
    }

    void doAccept() {
      acceptor.async_accept([self{shared_from_this()}](auto ec, auto socket) {
        if (!ec) {
          std::make_shared<HttpSession>(
              std::move(socket), self->rpc, self->routes)
              ->run();
        }
        self->doAccept();
      });
    }

    tcp::acceptor acceptor;
    std::shared_ptr<Rpc> rpc;
    std::shared_ptr<Routes> routes;
  };

  void serve(std::shared_ptr<Rpc> rpc,
             std::shared_ptr<Routes> routes,
             boost::asio::io_context &ioc,
             std::string_view ip,
             unsigned short port) {
    std::make_shared<Server>(
        tcp::acceptor{ioc, {net::ip::make_address(ip), port}},
        std::move(rpc),
        std::move(routes))
        ->run();
  }

  WrapperResponse &WrapperResponse::operator=(
      WrapperResponse &&other) noexcept {
    response = std::move(other.response);
    release_resources = std::move(other.release_resources);
    return *this;
  }

  WrapperResponse::~WrapperResponse() {
    if (release_resources) {
      release_resources();
    }
  }

  WrapperResponse::WrapperResponse(WrapperResponse &&other) noexcept {
    response = std::move(other.response);
    release_resources = std::move(other.release_resources);
  }

  WrapperResponse::WrapperResponse(ResponseType &&response)
      : response(std::move(response)) {}

  WrapperResponse::WrapperResponse(ResponseType &&response,
                                   std::function<void()> &&clear)
      : response(std::move(response)), release_resources(std::move(clear)) {}
}  // namespace fc::api
