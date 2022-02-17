/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/wsc.hpp"

#include "api/rpc/json.hpp"
#include "api/rpc/web_socket_client_error.hpp"
#include "codec/json/json.hpp"
#include "common/ptr.hpp"
#include "common/which.hpp"

namespace fc::api::rpc {
  Client::Client(io_context &io2)
      : io2{io2},
        work_guard{io.get_executor()},
        socket{io},
        logger_{common::createLogger("WebSocket Client")} {
    thread = std::thread{[=]() { io.run(); }};
  }

  Client::~Client() {
    io.stop();
    thread.join();
  }

  outcome::result<void> Client::connect(const Multiaddress &address,
                                        const std::string &target,
                                        const std::string &token) {
    OUTCOME_TRY(
        ip,
        address.getFirstValueForProtocol(libp2p::multi::Protocol::Code::IP4));
    OUTCOME_TRY(
        port,
        address.getFirstValueForProtocol(libp2p::multi::Protocol::Code::TCP));
    return connect(ip, port, target, token);
  }

    outcome::result<void> Client::connect(const std::string &host,
                                          const std::string &port,
                                          const std::string &target,
                                          const std::string &token) {
    boost::system::error_code ec;
    socket.next_layer().connect({boost::asio::ip::make_address(host),
                                 boost::lexical_cast<uint16_t>(port)},
                                ec);
    if (ec) {
      return ec;
    }
    if (not token.empty()) {
      socket.set_option(
          boost::beast::websocket::stream_base::decorator([&](auto &req) {
            req.set(boost::beast::http::field::authorization,
                    "Bearer " + token);
          }));
    }
    socket.handshake(host, target, ec);
    client_data = ClientData{host, port, target, token};
    if (ec) {
      return ec;
    }
    _read();
    return outcome::success();
  }

  void Client::call(Request &&req, ResultCb &&cb) {
    std::lock_guard lock{mutex};
    req.id = next_req++;
    write_queue.emplace(*req.id, *codec::json::format(encode(req)));
    result_queue.emplace(*req.id, std::move(cb));
    _flush();
  }

  void Client::_chan(uint64_t id, ChanCb &&cb) {
    chans.emplace(id, std::move(cb));
  }

  void Client::_error(const std::error_code &error) {
    write_queue = {};
    for (auto &[id, cb] : result_queue) {
      cb(error);
    }
    result_queue.clear();
    for (auto &[id, cb] : chans) {
      if (cb) {
        cb({});
      }
    }
    chans.clear();
    reconnect(3, std::chrono::seconds(5));
  }

  void Client::_flush() {
    if (!writing && !write_queue.empty() && !reconnecting){
      auto &[id, buffer] = write_queue.front();
      writing = true;
      socket.async_write(boost::asio::buffer(buffer.data(), buffer.size()),
                         [=](auto &&ec, auto) {
                           std::lock_guard lock{mutex};
                           if (ec) {
                             return _error(ec);
                           }
                           writing = false;
                           write_queue.pop();
                           _flush();
                         });
    }
  }

  void Client::_read() {
    socket.async_read(buffer, [=](auto &&ec, auto) {
      if (ec) {
        std::lock_guard lock{mutex};
        return _error(ec);
      }
      std::string_view s{static_cast<const char *>(buffer.cdata().data()),
                         buffer.cdata().size()};
      if (auto _req{codec::json::parse(s)}) {
        _onread(_req.value());
      }
      buffer.clear();
      _read();
    });
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  void Client::_onread(const Document &j) {
    if (j.HasMember("method")) {
      if (auto _req{decode<Request>(j)}) {
        auto &req{_req.value()};
        auto close{req.method == kRpcChClose};
        if (close || req.method == kRpcChVal) {
          boost::optional<uint64_t> id;
          boost::optional<Document> value;
          if (close) {
            if (auto _params{decode<std::tuple<uint64_t>>(req.params)}) {
              std::tie(id) = _params.value();
            }
          } else {
            if (auto _params{
                    decode<std::tuple<uint64_t, Document>>(req.params)}) {
              std::tie(id, value) = std::move(_params.value());
            }
          }
          if (id) {
            boost::asio::post(
                *thread_chan.io,
                [this, close, id{*id}, value{std::move(value)}]() mutable {
                  std::unique_lock lock{mutex};
                  auto it{chans.find(id)};
                  if (it != chans.end()) {
                    auto cb{std::move(it->second)};
                    lock.unlock();
                    if (!cb(std::move(value)) || close) {
                      lock.lock();
                      chans.erase(id);
                    } else {
                      lock.lock();
                      auto it{chans.find(id)};
                      if (it != chans.end()) {
                        it->second = std::move(cb);
                      }
                    }
                  }
                });
          }
        }
      }
    } else {
      if (auto _res{decode<Response>(j)}) {
        auto &res{_res.value()};
        if (res.id) {
          std::lock_guard lock{mutex};
          auto it{result_queue.find(*res.id)};
          if (it != result_queue.end()) {
            if (common::which<Document>(res.result)) {
              it->second(std::move(boost::get<Document>(res.result)));
            } else {
              auto err = boost::get<Response::Error>(res.result);
              logger_->warn("API error: {} {}", err.code, err.message);
              it->second(WebSocketClientError::kRpcErrorResponse);
            }
            result_queue.erase(it);
          }
        }
      }
    }
  }

  void Client::reconnect(int counter, std::chrono::milliseconds wait) {
    if(reconnecting.exchange(true)) return;
    logger_->info("Starting reconnect to {}:{}", client_data.host, client_data.port);
    for(int i = 0; i < counter; i++){
      std::this_thread::sleep_for(wait*(i+1));
      auto res = connect(client_data.host,
                         client_data.port,
                         client_data.target,
                         client_data.token);
      if(!res.has_error()) {
        break;
      }
    }
    reconnecting.store(false);
    logger_->info("Reconnect to {}:{} was successful", client_data.host, client_data.port);
    _flush();
  }
}  // namespace fc::api::rpc
