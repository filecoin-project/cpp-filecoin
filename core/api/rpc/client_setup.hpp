/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/rpc/wsc.hpp"
#include "common/ptr.hpp"

namespace fc::api::rpc {
  template <typename M>
  void Client::_setup(Client &c, M &m) {
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
}  // namespace fc::api::rpc
