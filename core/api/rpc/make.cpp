/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/make.hpp"
#include "api/rpc/json.hpp"

namespace fc::api {
  template <typename M>
  void setup(Rpc &rpc, const M &method) {
    rpc.setup(M::name, [&](auto &jparams) -> outcome::result<Document> {
      auto maybe_params = decode<typename M::Params>(jparams);
      if (!maybe_params) {
        return JsonError::WRONG_PARAMS;
      }
      auto maybe_result = std::apply(method, maybe_params.value());
      if (!maybe_result) {
        return maybe_result.error();
      }
      if constexpr (!std::is_same_v<typename M::Result, void>) {
        auto &result = maybe_result.value();
        // TODO: Chan
        return api::encode(result);
      }
      // TODO: void
      return Document{};
    });
  }

  void setupRpc(Rpc &rpc, const Api &api) {
    setup(rpc, api.ChainGetRandomness);
    setup(rpc, api.ChainHead);
    setup(rpc, api.ChainNotify);
    setup(rpc, api.ChainReadObj);
    setup(rpc, api.ChainTipSetWight);
    setup(rpc, api.MarketEnsureAvailable);
    setup(rpc, api.MinerCreateBlock);
    setup(rpc, api.MpoolPending);
    setup(rpc, api.MpoolPushMessage);
    setup(rpc, api.PaychVoucherAdd);
    setup(rpc, api.StateCall);
    setup(rpc, api.StateGetActor);
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
    setup(rpc, api.WalletSign);
  }
}  // namespace fc::api
