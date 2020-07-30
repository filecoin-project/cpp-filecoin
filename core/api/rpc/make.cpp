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
        [&](auto &jparams,
            rpc::Respond respond,
            rpc::MakeChan make_chan,
            rpc::Send send) {
          auto maybe_params = decode<typename M::Params>(jparams);
          if (!maybe_params) {
            return respond(Response::Error{kInvalidParams,
                                           maybe_params.error().message()});
          }
          auto maybe_result = std::apply(method, maybe_params.value());
          if (!maybe_result) {
            return respond(Response::Error{kInternalError,
                                           maybe_result.error().message()});
          }
          if constexpr (!std::is_same_v<Result, void>) {
            auto &result = maybe_result.value();
            if constexpr (is_chan<Result>{}) {
              result.id = make_chan();
            }
            if constexpr (is_wait<Result>{}) {
              result.wait([respond{std::move(respond)}](auto maybe_result) {
                if (!maybe_result) {
                  respond(Response::Error{kInternalError,
                                          maybe_result.error().message()});
                } else {
                  respond(encode(maybe_result.value()));
                }
              });
              return;
            } else {
              respond(api::encode(result));
            }
            if constexpr (is_chan<Result>{}) {
              result.channel->read([send{std::move(send)},
                                    chan{result}](auto opt) {
                if (opt) {
                  send("xrpc.ch.val",
                       encode(std::make_tuple(chan.id, std::move(*opt))),
                       [chan](auto ok) {
                         if (!ok) {
                           chan.channel->closeRead();
                         }
                       });
                } else {
                  send("xrpc.ch.close", encode(std::make_tuple(chan.id)), {});
                }
                return true;
              });
            }
            return;
          }
          respond(Document{});
        });
  }

  void setupRpc(Rpc &rpc, const Api &api) {
    setup(rpc, api.AuthNew);
    setup(rpc, api.ChainGetBlock);
    setup(rpc, api.ChainGetBlockMessages);
    setup(rpc, api.ChainGetGenesis);
    setup(rpc, api.ChainGetNode);
    setup(rpc, api.ChainGetMessage);
    setup(rpc, api.ChainGetParentMessages);
    setup(rpc, api.ChainGetParentReceipts);
    setup(rpc, api.ChainGetTipSet);
    setup(rpc, api.ChainGetTipSetByHeight);
    setup(rpc, api.ChainHead);
    setup(rpc, api.ChainNotify);
    setup(rpc, api.ChainReadObj);
    setup(rpc, api.ChainSetHead);
    setup(rpc, api.ChainTipSetWeight);
    setup(rpc, api.ClientFindData);
    setup(rpc, api.ClientHasLocal);
    setup(rpc, api.ClientImport);
    setup(rpc, api.ClientListImports);
    setup(rpc, api.ClientQueryAsk);
    setup(rpc, api.ClientRetrieve);
    setup(rpc, api.ClientStartDeal);
    setup(rpc, api.MarketEnsureAvailable);
    setup(rpc, api.MinerCreateBlock);
    setup(rpc, api.MinerGetBaseInfo);
    setup(rpc, api.MpoolPending);
    setup(rpc, api.MpoolPushMessage);
    setup(rpc, api.MpoolSub);
    setup(rpc, api.NetAddrsListen);
    setup(rpc, api.StateAccountKey);
    setup(rpc, api.StateCall);
    setup(rpc, api.StateListMessages);
    setup(rpc, api.StateGetActor);
    setup(rpc, api.StateReadState);
    setup(rpc, api.StateGetReceipt);
    setup(rpc, api.StateListMiners);
    setup(rpc, api.StateListActors);
    setup(rpc, api.StateMarketBalance);
    setup(rpc, api.StateMarketDeals);
    setup(rpc, api.StateLookupID);
    setup(rpc, api.StateMarketStorageDeal);
    setup(rpc, api.StateMinerDeadlines);
    setup(rpc, api.StateMinerFaults);
    setup(rpc, api.StateMinerInfo);
    setup(rpc, api.StateMinerPower);
    setup(rpc, api.StateMinerProvingDeadline);
    setup(rpc, api.StateMinerProvingSet);
    setup(rpc, api.StateMinerSectors);
    setup(rpc, api.StateMinerSectorSize);
    setup(rpc, api.StateMinerWorker);
    setup(rpc, api.StateNetworkName);
    setup(rpc, api.StateWaitMsg);
    setup(rpc, api.SyncSubmitBlock);
    setup(rpc, api.Version);
    setup(rpc, api.WalletBalance);
    setup(rpc, api.WalletDefaultAddress);
    setup(rpc, api.WalletHas);
    setup(rpc, api.WalletSign);
    setup(rpc, api.WalletVerify);

    setup(rpc, api.PaychAllocateLane);
    setup(rpc, api.PaychGet);
    setup(rpc, api.PaychVoucherAdd);
    setup(rpc, api.PaychVoucherCheckValid);
    setup(rpc, api.PaychVoucherCreate);
  }
}  // namespace fc::api
