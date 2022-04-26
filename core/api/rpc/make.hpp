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
  using rpc::AuthFunction;
  using rpc::Rpc;

  template <typename A>
  std::shared_ptr<Rpc> makeRpc(A &&api) {
    return makeRpc(std::move(api), {});
  }

  template <typename A>
  std::shared_ptr<Rpc> makeRpc(A &&api, AuthFunction &&auth) {
    std::shared_ptr<Rpc> rpc = std::make_shared<Rpc>(std::move(auth));
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
        method.getName(),
        [&](auto &jparams,
            rpc::Respond respond,
            rpc::MakeChan make_chan,
            rpc::Send send,
            const rpc::Permissions &perms) {
          if (not primitives::jwt::hasPermission(perms, method.getPerm())) {
            return respond(Response::Error{kInvalidParams,
                                           "Missing permission to invoke"});
          }

          auto maybe_params = codec::json::decode<typename M::Params>(jparams);
          if (!maybe_params) {
            return respond(Response::Error{
                kInvalidParams, errorToPrettyString(maybe_params.error())});
          }

          typename M::Callback cb =
              [respond{std::move(respond)},
               make_chan{std::move(make_chan)},
               send{std::move(send)}](
                  const outcome::result<Result> &maybe_result) mutable {
                if (!maybe_result) {
                  return respond(Response::Error{
                      kInternalError,
                      errorToPrettyString(maybe_result.error())});
                }
                if constexpr (!std::is_same_v<Result, void>) {
                  if constexpr (is_chan<Result>{}) {
                    Result result = std::move(maybe_result.value());
                    result.id = make_chan();
                    respond(codec::json::encode(result));
                    result.channel->read([send{std::move(send)},
                                          chan{result}](auto opt) {
                      if (opt) {
                        send(kRpcChVal,
                             codec::json::encode(std::make_tuple(chan.id, std::move(*opt))),
                             [chan](auto ok) {
                               if (!ok) {
                                 chan.channel->closeRead();
                               }
                             });
                      } else {
                        send(kRpcChClose, codec::json::encode(std::make_tuple(chan.id)), {});
                      }
                      return true;
                    });
                  } else {
                    respond(codec::json::encode(maybe_result.value()));
                  }
                } else {
                  respond(Document{});
                }
              };
          auto params =
              std::tuple_cat(std::make_tuple(cb), maybe_params.value());
          std::apply(method, params);
        });
  }
}  // namespace fc::api
