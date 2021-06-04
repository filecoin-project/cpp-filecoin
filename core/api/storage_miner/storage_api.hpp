/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/common_api.hpp"
#include "api/full_node/node_api.hpp"
#include "api/version.hpp"
#include "markets/retrieval/provider/retrieval_provider.hpp"
#include "markets/retrieval/types.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/provider/provider.hpp"
#include "markets/storage/provider/stored_ask.hpp"
#include "miner/miner.hpp"
#include "miner/storage_fsm/types.hpp"
#include "sector_storage/manager.hpp"
#include "sector_storage/scheduler.hpp"
#include "sector_storage/stores/index.hpp"

namespace fc::api {
  using boost::asio::io_context;
  using markets::retrieval::RetrievalAsk;
  using markets::retrieval::provider::RetrievalProvider;
  using markets::storage::SignedStorageAsk;
  using markets::storage::provider::StorageProvider;
  using markets::storage::provider::StoredAsk;
  using miner::Miner;
  using mining::types::DealInfo;
  using mining::types::DealSchedule;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::StorageID;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;
  using primitives::sector_file::SectorFileType;
  using sector_storage::Manager;
  using sector_storage::Scheduler;
  using sector_storage::stores::FsStat;
  using sector_storage::stores::HealthReport;
  using sector_storage::stores::SectorIndex;
  using StorageInfo_ = sector_storage::stores::StorageInfo;

  struct PieceLocation {
    SectorNumber sector_number;
    PaddedPieceSize offset;
    PaddedPieceSize length;
  };

  inline bool operator==(const PieceLocation &lhs, const PieceLocation &rhs) {
    return lhs.sector_number == rhs.sector_number && lhs.offset == rhs.offset
           && lhs.length == rhs.length;
  }

  constexpr ApiVersion kMinerApiVersion = makeApiVersion(1, 0, 0);

  /**
   * Storage miner node low-level interface API.
   */
  struct StorageMinerApi : public CommonApi {
    API_METHOD(ActorAddress, Address)

    API_METHOD(ActorSectorSize, SectorSize, const Address &)

    API_METHOD(PledgeSector, void)

    /**
     * Manually import data for a storage deal
     * @param proposal CID
     * @param path to a file with piece data
     */
    API_METHOD(DealsImportData, void, const CID &, const std::string &)

    API_METHOD(MarketGetAsk, SignedStorageAsk)
    API_METHOD(MarketGetRetrievalAsk, RetrievalAsk)
    API_METHOD(MarketSetAsk,
               void,
               const TokenAmount &,
               const TokenAmount &,
               ChainEpoch,
               PaddedPieceSize,
               PaddedPieceSize)

    API_METHOD(MarketSetRetrievalAsk, void, const RetrievalAsk &)

    API_METHOD(StorageAttach, void, const StorageInfo_ &, const FsStat &)
    API_METHOD(StorageInfo, StorageInfo_, const StorageID &)
    API_METHOD(StorageReportHealth,
               void,
               const StorageID &,
               const HealthReport &)
    API_METHOD(StorageDeclareSector,
               void,
               const StorageID &,
               const SectorId &,
               const SectorFileType &,
               bool)
    API_METHOD(StorageDropSector,
               void,
               const StorageID &,
               const SectorId &,
               const SectorFileType &)
    API_METHOD(StorageFindSector,
               std::vector<StorageInfo_>,
               const SectorId &,
               const SectorFileType &,
               boost::optional<SectorSize>)
    API_METHOD(StorageBestAlloc,
               std::vector<StorageInfo_>,
               const SectorFileType &,
               SectorSize,
               bool)

    API_METHOD(WorkerConnect, void, const std::string &);
  };

  /**
   * Creates StorageMinerApi implementation
   * @return initialized StorageMinerApi
   */
  std::shared_ptr<StorageMinerApi> makeStorageApi(
      const std::shared_ptr<io_context> io,
      const std::shared_ptr<FullNodeApi> &full_node_api,
      const Address &actor,
      const std::shared_ptr<Miner> &miner,
      const std::shared_ptr<SectorIndex> &sector_index,
      const std::shared_ptr<Manager> &sector_manager,
      const std::shared_ptr<Scheduler> &sector_scheduler,
      const std::shared_ptr<StoredAsk> &stored_ask,
      const std::shared_ptr<StorageProvider> &storage_market_provider,
      const std::shared_ptr<RetrievalProvider> &retrieval_market_provider);

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
    f(a.StorageAttach);
    f(a.StorageInfo);
    f(a.StorageReportHealth);
    f(a.StorageDeclareSector);
    f(a.StorageDropSector);
    f(a.StorageFindSector);
    f(a.StorageBestAlloc);
    f(a.WorkerConnect);
  }
}  // namespace fc::api
