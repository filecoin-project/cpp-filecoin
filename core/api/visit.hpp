/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc::api {
  struct FullNodeApi;
  struct StorageMinerApi;
  struct WorkerApi;

  template <typename A, typename F>
  void visitCommon(A &&a, const F &f) {
    f(a.AuthNew);
    f(a.NetAddrsListen);
    f(a.NetConnect);
    f(a.NetPeers);
    f(a.Version);
  }

  template <typename A, typename F>
  void visit(A &&a, const F &f) {
    visit(a, a, f);
  }

  template <typename A, typename F>
  void visit(const StorageMinerApi &, A &&a, const F &f) {
    visitCommon(a, f);
    f(a.ActorAddress);
    f(a.ActorSectorSize);
    f(a.PledgeSector);
    f(a.DealsImportData);
    f(a.MarketGetAsk);
    f(a.MarketGetRetrievalAsk);
    f(a.MarketSetAsk);
    f(a.MarketSetRetrievalAsk);
    f(a.SealProof);  // TODO(ortyomka): [FIL-347] remove it
    f(a.StorageAttach);
    f(a.StorageInfo);
    f(a.StorageReportHealth);
    f(a.StorageDeclareSector);
    f(a.StorageDropSector);
    f(a.StorageFindSector);
    f(a.StorageBestAlloc);
    f(a.WorkerConnect);
  }

  template <typename A, typename F>
  void visit(const WorkerApi &, A &&a, const F &f) {
    f(a.Fetch);
    f(a.FinalizeSector);
    f(a.Info);
    f(a.MoveStorage);
    f(a.Paths);
    f(a.Remove);
    f(a.SealCommit1);
    f(a.SealCommit2);
    f(a.SealPreCommit1);
    f(a.SealPreCommit2);
    f(a.StorageAddLocal);
    f(a.TaskTypes);
    f(a.UnsealPiece);
    f(a.Version);
  }

  template <typename A, typename F>
  void visit(const FullNodeApi &, A &&a, const F &f) {
    visitCommon(a, f);
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
    f(a.ClientListDeals);
    f(a.ClientListImports);
    f(a.ClientQueryAsk);
    f(a.ClientRetrieve);
    f(a.ClientStartDeal);
    f(a.GasEstimateFeeCap);
    f(a.GasEstimateGasPremium);
    f(a.GasEstimateMessageGas);
    f(a.MarketReserveFunds);
    f(a.MinerCreateBlock);
    f(a.MinerGetBaseInfo);
    f(a.MpoolPending);
    f(a.MpoolPushMessage);
    f(a.MpoolSelect);
    f(a.MpoolSub);
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
    f(a.StateNetworkVersion);
    f(a.StateReadState);
    f(a.StateSearchMsg);
    f(a.StateSectorGetInfo);
    f(a.StateSectorPartition);
    f(a.StateVerifiedClientStatus);
    f(a.GetProofType);
    f(a.StateSectorPreCommitInfo);
    f(a.StateWaitMsg);
    f(a.SyncSubmitBlock);
    f(a.WalletBalance);
    f(a.WalletDefaultAddress);
    f(a.WalletHas);
    f(a.WalletImport);
    f(a.WalletSign);
    f(a.WalletVerify);
  }
}  // namespace fc::api
