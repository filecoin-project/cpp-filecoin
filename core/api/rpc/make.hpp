/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/rpc/json.hpp"
#include "api/rpc/rpc.hpp"
#include "api/utils.hpp"
#include "api/visit.hpp"
#include "common/outcome_fmt.hpp"

namespace fc::api {
  using outcome::errorToPrettyString;
  using rpc::Rpc;

  template <typename A>
  std::shared_ptr<Rpc> makeRpc(A &&api) {
    std::shared_ptr<Rpc> rpc = std::make_shared<Rpc>();
    api::visit(api, [&](auto &m) { setup(*rpc, m); });
    return rpc;
  }

  /**
   * Updates RPC by overriding API methods.
   * @tparam A - updated API type with methods that should be overridden
   * @param rpc - RPC to update
   * @param api - API that contains methods to override
   */
  template <typename A>
  void wrapRpc(const std::shared_ptr<Rpc> &rpc, A &&api_wrapper) {
    api::visit(api_wrapper, [&](auto &m) { setup(*rpc, m); });
  }

  template <typename M>
  void setup(Rpc &rpc, const M &method) {
    if (!method) return;
    using Result = typename M::Result;
    rpc.setup(
        M::name,
        [&](auto &jparams,
            rpc::Respond respond,
            rpc::MakeChan make_chan,
            rpc::Send send) {
          auto maybe_params = decode<typename M::Params>(jparams);
          if (!maybe_params) {
            return respond(Response::Error{
                kInvalidParams, errorToPrettyString(maybe_params.error())});
          }
          auto maybe_result = std::apply(method, maybe_params.value());
          if (!maybe_result) {
            return respond(Response::Error{
                kInternalError, errorToPrettyString(maybe_result.error())});
          }
          if constexpr (!std::is_same_v<Result, void>) {
            auto &result = maybe_result.value();
            if constexpr (is_chan<Result>{}) {
              result.id = make_chan();
            }
            if constexpr (is_wait<Result>{}) {
              result.waitOwn([respond{std::move(respond)}](auto maybe_result) {
                if (!maybe_result) {
                  respond(Response::Error{
                      kInternalError,
                      errorToPrettyString(maybe_result.error())});
                } else {
                  respond(encode(maybe_result.value()));
                }
              });
              return;
            } else {
              respond(api::encode(result));
            }
            if constexpr (is_chan<Result>{}) {
              result.channel->read(
                  [send{std::move(send)}, chan{result}](auto opt) {
                    if (opt) {
                      send(kRpcChVal,
                           encode(std::make_tuple(chan.id, std::move(*opt))),
                           [chan](auto ok) {
                             if (!ok) {
                               chan.channel->closeRead();
                             }
                           });
                    } else {
                      send(kRpcChClose, encode(std::make_tuple(chan.id)), {});
                    }
                    return true;
                  });
            }
            return;
          }
          respond(Document{});
        });
  }
}  // namespace fc::api
