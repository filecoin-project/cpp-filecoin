/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/ws.hpp"

#include <rapidjson/writer.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "api/rpc/json.hpp"
#include "api/rpc/make.hpp"

namespace fc::api {
  namespace beast = boost::beast;
  namespace http = beast::http;
  namespace websocket = beast::websocket;
  namespace net = boost::asio;
  using tcp = boost::asio::ip::tcp;

  constexpr auto kParseError = INT64_C(-32700);
  constexpr auto kInvalidRequest = INT64_C(-32600);
  constexpr auto kMethodNotFound = INT64_C(-32601);
  constexpr auto kInvalidParams = INT64_C(-32602);
  constexpr auto kInternalError = INT64_C(-32603);

  struct ServerSession : std::enable_shared_from_this<ServerSession> {
    ServerSession(tcp::socket &&socket, const Api &api)
        : socket{std::move(socket)} {
      setupRpc(rpc, api);
    }

    void run() {
      socket.async_accept([self{shared_from_this()}](auto ec) {
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
      auto res = handle({static_cast<const char *>(buffer.cdata().data()),
                         buffer.cdata().size()});
      buffer.clear();
      rapidjson::StringBuffer j_buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer{j_buffer};
      encode(res).Accept(writer);
      socket.async_write(net::buffer(std::string_view{j_buffer.GetString(),
                                                      j_buffer.GetSize()}),
                         [](auto, auto) {});
    }

    Response handle(std::string_view s_req) {
      rapidjson::Document j_req;
      j_req.Parse(static_cast<const char *>(buffer.cdata().data()),
                  buffer.cdata().size());
      if (j_req.HasParseError()) {
        return {{}, Response::Error{kParseError, "Parse error"}};
      }
      auto maybe_req = decode<Request>(j_req);
      if (!maybe_req) {
        return {{}, Response::Error{kInvalidRequest, "Invalid request"}};
      }
      auto &req = maybe_req.value();
      auto it = rpc.ms.find(req.method);
      if (it == rpc.ms.end() || !it->second) {
        return {req.id, Response::Error{kMethodNotFound, "Method not found"}};
      }
      auto maybe_result = it->second(req.params);
      if (!maybe_result) {
        if (maybe_result.error() == JsonError::WRONG_PARAMS) {
          return {req.id, Response::Error{kInvalidParams, "Invalid params"}};
        }
        return {req.id, Response::Error{kInternalError, "Internal error"}};
      }
      return {req.id, std::move(maybe_result.value())};
    }

    websocket::stream<tcp::socket> socket;
    beast::flat_buffer buffer;
    Rpc rpc;
  };

  struct Server : std::enable_shared_from_this<Server> {
    Server(tcp::acceptor &&acceptor, Api api)
        : acceptor{std::move(acceptor)}, api{api} {}

    void run() {
      doAccept();
    }

    void doAccept() {
      acceptor.async_accept([self{shared_from_this()}](auto ec, auto socket) {
        if (ec) {
          return;
        }
        std::make_shared<ServerSession>(std::move(socket), self->api)->run();
        self->doAccept();
      });
    }

    tcp::acceptor acceptor;
    Api api;
  };

  void serve(Api api,
             boost::asio::io_context &ioc,
             std::string_view ip,
             unsigned short port) {
    std::make_shared<Server>(
        tcp::acceptor{ioc, {net::ip::make_address(ip), port}}, std::move(api))
        ->run();
  }
}  // namespace fc::api
