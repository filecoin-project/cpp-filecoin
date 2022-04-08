/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/rpc/wsc.hpp"
#include "common/ptr.hpp"

namespace fc::api::rpc {
  template <typename M>
  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  void Client::_setup(Client &c, M &m) {
    using Result = typename M::Result;
    using Callback = typename M::Callback;
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    m = [&c, name{m.getName()}](Callback cb, auto &&...params) -> void {
      Request req{};
      req.method = name;
      req.params = codec::json::encode(
          std::make_tuple(std::forward<decltype(params)>(params)...));
      if constexpr (is_chan<Result>{}) {
        auto chan{Result::make()};
        c.call(std::move(req),
               // NOLINTNEXTLINE(readability-function-cognitive-complexity)
               [&c, weak{weaken(chan.channel)}](auto &&_result) {
                 if (auto channel{weak.lock()}) {
                   if (_result) {
                     if (auto _chan{
                             codec::json::decode<Result>(_result.value())}) {
                       return c._chan(_chan.value().id, [weak](auto &&value) {
                         if (auto channel{weak.lock()}) {
                           if (value) {
                             if (auto _result{
                                     codec::json::decode<typename Result::Type>(
                                         *value)}) {
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
        cb(chan);
      } else {
        c.call(std::move(req), [&c, cb{std::move(cb)}](auto &&_result) {
          boost::asio::post(c.io2,
                            [_result{std::forward<decltype(_result)>(_result)},
                             cb{std::move(cb)}] {
                              OUTCOME_CB(auto &&result, _result);
                              cb(codec::json::decode<Result>(result));
                            });
        });
      }
    };
  }
}  // namespace fc::api::rpc
