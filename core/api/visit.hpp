/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc::api {
  template <typename A, typename F>
  void visit(A &&a, const F &f) {
    f(a.AuthNew);
    f(a.BeaconGetEntry);
    f(a.ChainGetBlock);
    f(a.ChainGetBlockMessages);
    f(a.ChainGetGenesis);
    f(a.ChainGetMessage);
    f(a.ChainGetNode);
    f(a.ChainGetParentMessages);
    f(a.ChainGetParentReceipts);
    f(a.ChainGetRandomnessFromBeacon);
    f(a.ChainGetRandomnessFromTickets);
    f(a.ChainGetTipSet);
    f(a.ChainGetTipSetByHeight);
    f(a.ChainHead);
    f(a.ChainNotify);
    f(a.ChainReadObj);
    f(a.ChainSetHead);
    f(a.ChainTipSetWeight);
    f(a.ClientFindData);
    f(a.ClientHasLocal);
    f(a.ClientImport);
    f(a.ClientListImports);
    f(a.ClientQueryAsk);
    f(a.ClientRetrieve);
    f(a.ClientStartDeal);
    f(a.MarketEnsureAvailable);
    f(a.MinerCreateBlock);
    f(a.MinerGetBaseInfo);
    f(a.MpoolPending);
    f(a.MpoolPushMessage);
    f(a.MpoolSelect);
    f(a.MpoolSub);
    f(a.NetAddrsListen);
    f(a.PaychAllocateLane);
    f(a.PaychGet);
    f(a.PaychVoucherAdd);
    f(a.PaychVoucherCheckValid);
    f(a.PaychVoucherCreate);
    f(a.StateAccountKey);
    f(a.StateCall);
    f(a.StateGetActor);
    f(a.StateGetReceipt);
    f(a.StateListActors);
    f(a.StateListMessages);
    f(a.StateListMiners);
    f(a.StateLookupID);
    f(a.StateMarketBalance);
    f(a.StateMarketDeals);
    f(a.StateMarketStorageDeal);
    f(a.StateMinerDeadlines);
    f(a.StateMinerFaults);
    f(a.StateMinerInfo);
    f(a.StateMinerInitialPledgeCollateral);
    f(a.StateMinerPartitions);
    f(a.StateMinerPower);
    f(a.StateMinerPreCommitDepositForPower);
    f(a.StateMinerProvingDeadline);
    f(a.StateMinerSectors);
    f(a.StateNetworkName);
    f(a.StateReadState);
    f(a.StateSearchMsg);
    f(a.StateSectorGetInfo);
    f(a.StateSectorPartition);
    f(a.StateSectorPreCommitInfo);
    f(a.StateWaitMsg);
    f(a.SyncSubmitBlock);
    f(a.Version);
    f(a.WalletBalance);
    f(a.WalletDefaultAddress);
    f(a.WalletHas);
    f(a.WalletImport);
    f(a.WalletSign);
    f(a.WalletVerify);
  }
}  // namespace fc::api
