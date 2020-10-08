/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/wsc.hpp"

#include "api/rpc/json.hpp"
#include "api/visit.hpp"
#include "codec/json/json.hpp"
#include "common/which.hpp"

// TODO: move to other file
template <typename T>
std::weak_ptr<T> weaken(const std::shared_ptr<T> &ptr) {
  return ptr;
}

namespace fc::api::rpc {
  Client::Client(io_context &io2)
      : io2{io2}, work_guard{io.get_executor()}, socket{io} {
    thread = std::thread{[=]() { io.run(); }};
  }

  outcome::result<void> Client::connect(const Multiaddress &address,
                                        const std::string &token) {
    OUTCOME_TRY(
        ip,
        address.getFirstValueForProtocol(libp2p::multi::Protocol::Code::IP4));
    OUTCOME_TRY(
        port,
        address.getFirstValueForProtocol(libp2p::multi::Protocol::Code::TCP));
    boost::system::error_code ec;
    socket.next_layer().connect(
        {boost::asio::ip::make_address(ip), (unsigned short)std::stoul(port)},
        ec);
    if (ec) {
      return ec;
    }
    socket.set_option(
        boost::beast::websocket::stream_base::decorator([&](auto &req) {
          req.set(boost::beast::http::field::authorization, "Bearer " + token);
        }));
    socket.handshake(ip, "/rpc/v0", ec);
    if (ec) {
      return ec;
    }
    _read();
    return outcome::success();
  }

  void Client::call(Request &&req, ResultCb &&cb) {
    std::lock_guard lock{mutex};
    req.id = next_req++;
    auto j{encode(req)};
    write_queue.emplace(*req.id, *codec::json::format(&j));
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
  }

  void Client::_flush() {
    if (!writing && !write_queue.empty()) {
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
                io2, [this, close, id{*id}, value{std::move(value)}]() mutable {
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
              it->second(std::errc::owner_dead);
            }
            result_queue.erase(it);
          }
        }
      }
    }
  }

  template <typename M>
  void _setup(Client &c, M &m) {
    using Result = typename M::Result;
    m = [&c](auto &&... params) -> outcome::result<Result> {
      Request req{};
      req.method = M::name;
      req.params =
          encode(std::make_tuple(std::forward<decltype(params)>(params)...));
      if constexpr (is_wait<Result>{}) {
        auto wait{Result::make()};
        c.call(std::move(req),
               [&c, weak{weaken(wait.channel)}](auto &&_result) {
                 boost::asio::post(c.io2, [weak, _result{std::move(_result)}] {
                   if (auto channel{weak.lock()}) {
                     if (_result) {
                       channel->write(
                           api::decode<typename Result::Type>(_result.value()));
                     } else {
                       channel->write(_result.error());
                     }
                   }
                 });
               });
        return wait;
      } else if constexpr (is_chan<Result>{}) {
        auto chan{Result::make()};
        c.call(
            std::move(req), [&c, weak{weaken(chan.channel)}](auto &&_result) {
              if (auto channel{weak.lock()}) {
                if (_result) {
                  if (auto _chan{api::decode<Result>(_result.value())}) {
                    return c._chan(_chan.value().id, [weak](auto &&value) {
                      if (auto channel{weak.lock()}) {
                        if (value) {
                          if (auto _result{
                                  api::decode<typename Result::Type>(*value)}) {
                            if (channel->write(std::move(_result.value()))) {
                              return true;
                            }
                          }
                        } else {
                          channel->closeWrite();
                        }
                      }
                      return false;
                    });
                  }
                }
                channel->closeWrite();
              }
            });
        return chan;
      } else {
        std::promise<outcome::result<Result>> promise;
        auto future{promise.get_future()};
        c.call(std::move(req), [&promise](auto &&_result) {
          if (_result) {
            promise.set_value(api::decode<Result>(_result.value()));
          } else {
            promise.set_value(_result.error());
          }
        });
        return future.get();
      }
    };
  }

  void Client::setup(Api &api) {
    visit(api, [&](auto &m) { _setup(*this, m); });
  }
}  // namespace fc::api::rpc
