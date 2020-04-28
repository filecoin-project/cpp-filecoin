/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/make.hpp"
#include "api/rpc/json.hpp"

namespace fc::api {
  template <typename M>
  void setup(Rpc &rpc, const M &method) {
    using Result = typename M::Result;
    rpc.setup(
        M::name,
        [&](auto &jparams, auto make_chan) -> outcome::result<Document> {
          auto maybe_params = decode<typename M::Params>(jparams);
          if (!maybe_params) {
            return JsonError::WRONG_PARAMS;
          }
          auto maybe_result = std::apply(method, maybe_params.value());
          if (!maybe_result) {
            return maybe_result.error();
          }
          if constexpr (!std::is_same_v<Result, void>) {
            auto &result = maybe_result.value();
            if constexpr (is_chan<Result>{}) {
              result.id = make_chan([chan{result}](auto send) {
                chan.channel->read([send{std::move(send)}, chan](auto opt) {
                  if (opt) {
                    send(Request{0,
                                 "xrpc.ch.val",
                                 encode(std::make_tuple(chan.id,
                                                        std::move(*opt)))},
                         [chan](auto ok) {
                           if (!ok) {
                             chan.channel->closeRead();
                           }
                         });
                  } else {
                    send(Request{0,
                                 "xrpc.ch.close",
                                 encode(std::make_tuple(chan.id))},
                         [](auto) {});
                  }
                  return true;
                });
              });
            }
            return api::encode(result);
          }
          return Document{};
        });
  }

  void setupRpc(Rpc &rpc, const Api &api) {
    setup(rpc, api.ChainGetBlock);
    setup(rpc, api.ChainGetBlockMessages);
    setup(rpc, api.ChainGetGenesis);
    setup(rpc, api.ChainGetNode);
    setup(rpc, api.ChainGetParentMessages);
    setup(rpc, api.ChainGetParentReceipts);
    setup(rpc, api.ChainGetRandomness);
    setup(rpc, api.ChainGetTipSet);
    setup(rpc, api.ChainGetTipSetByHeight);
    setup(rpc, api.ChainHead);
    setup(rpc, api.ChainNotify);
    setup(rpc, api.ChainReadObj);
    setup(rpc, api.ChainSetHead);
    setup(rpc, api.ChainTipSetWeight);
    setup(rpc, api.MarketEnsureAvailable);
    setup(rpc, api.MinerCreateBlock);
    setup(rpc, api.MpoolPending);
    setup(rpc, api.MpoolPushMessage);
    setup(rpc, api.MpoolSub);
    setup(rpc, api.PaychVoucherAdd);
    setup(rpc, api.StateAccountKey);
    setup(rpc, api.StateCall);
    setup(rpc, api.StateGetActor);
    setup(rpc, api.StateListMiners);
    setup(rpc, api.StateMarketBalance);
    setup(rpc, api.StateMarketDeals);
    setup(rpc, api.StateMarketStorageDeal);
    setup(rpc, api.StateMinerElectionPeriodStart);
    setup(rpc, api.StateMinerFaults);
    setup(rpc, api.StateMinerPower);
    setup(rpc, api.StateMinerProvingSet);
    setup(rpc, api.StateMinerSectorSize);
    setup(rpc, api.StateMinerWorker);
    setup(rpc, api.StateWaitMsg);
    setup(rpc, api.SyncSubmitBlock);
    setup(rpc, api.WalletDefaultAddress);
    setup(rpc, api.WalletSign);
  }
}  // namespace fc::api
