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
  using mining::types::Piece;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::StorageID;
  using primitives::TokenAmount;
  using primitives::DealWeight;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;
  using primitives::sector::SectorId;
  using primitives::sector_file::SectorFileType;
  using sector_storage::CallError;
  using sector_storage::CallId;
  using sector_storage::Commit1Output;
  using sector_storage::Manager;
  using sector_storage::PieceInfo;
  using sector_storage::PreCommit1Output;
  using sector_storage::Proof;
  using sector_storage::Scheduler;
  using sector_storage::SectorCids;
  using sector_storage::stores::FsStat;
  using sector_storage::stores::HealthReport;
  using sector_storage::stores::SectorIndex;
  using StorageInfo_ = sector_storage::stores::StorageInfo;

  struct PieceLocation {
    SectorNumber sector_number;
    PaddedPieceSize offset;
    PaddedPieceSize length;
  };
  CBOR_TUPLE(PieceLocation, sector_number, offset, length)

  // TODO(ortyomka): [FIL-421] implement it
  struct ApiSectorInfo {
    mining::SealingState state = mining::SealingState::kStateUnknown;
    SectorNumber sector_id;
    RegisteredSealProof sector_type;
    boost::optional<CID> comm_d;
    boost::optional<CID> comm_r;
    proofs::Proof proof;
    std::vector<DealId> deals;
    proofs::SealRandomness ticket;
    sector_storage::InteractiveRandomness seed;
    boost::optional<CID> precommit_message;
    boost::optional<CID> commit_message;
    int64_t retries;
    bool to_upgrade;

    // On chain info

    RegisteredSealProof seal_proof = {};
    ChainEpoch activation = {};
    ChainEpoch expiration = {};
    DealWeight deal_weight = {};
    DealWeight verified_deal_weight = {};
    TokenAmount initial_pledge = {};

    ChainEpoch on_time = {};
    ChainEpoch early = {};
  };
  CBOR_TUPLE(ApiSectorInfo,
             state,
             sector_id,
             sector_type,
             ticket,
             comm_d,
             comm_r,
             proof,
             deals,
             ticket,
             seed,
             precommit_message,
             commit_message,
             retries,
             to_upgrade,
             seal_proof,
             activation,
             expiration,
             deal_weight,
             verified_deal_weight,
             initial_pledge,
             on_time
             )

  inline bool operator==(const PieceLocation &lhs, const PieceLocation &rhs) {
    return lhs.sector_number == rhs.sector_number && lhs.offset == rhs.offset
           && lhs.length == rhs.length;
  }

  constexpr ApiVersion kMinerApiVersion = makeApiVersion(1, 0, 0);

  /**
   * Storage miner node low-level interface API.
   */
  struct StorageMinerApi : public CommonApi {
    API_METHOD(ActorAddress, jwt::kReadPermission, Address)

    API_METHOD(ActorSectorSize,
               jwt::kReadPermission,
               SectorSize,
               const Address &)

    API_METHOD(PledgeSector, jwt::kWritePermission, void)

    /**
     * Manually import data for a storage deal
     * @param proposal CID
     * @param path to a file with piece data
     */
    API_METHOD(DealsImportData,
               jwt::kAdminPermission,
               void,
               const CID &,
               const std::string &)

    API_METHOD(MarketGetAsk, jwt::kReadPermission, SignedStorageAsk)
    API_METHOD(MarketGetRetrievalAsk, jwt::kReadPermission, RetrievalAsk)
    API_METHOD(MarketSetAsk,
               jwt::kAdminPermission,
               void,
               const TokenAmount &,
               const TokenAmount &,
               ChainEpoch,
               PaddedPieceSize,
               PaddedPieceSize)

    API_METHOD(MarketSetRetrievalAsk,
               jwt::kAdminPermission,
               void,
               const RetrievalAsk &)

    API_METHOD(SectorsList, jwt::kReadPermission, std::vector<SectorNumber>)

    API_METHOD(
        SectorsStatus, jwt::kReadPermission, ApiSectorInfo, SectorNumber, bool)

    API_METHOD(StorageAttach,
               jwt::kAdminPermission,
               void,
               const StorageInfo_ &,
               const FsStat &)
    API_METHOD(StorageInfo,
               jwt::kAdminPermission,
               StorageInfo_,
               const StorageID &)
    API_METHOD(StorageReportHealth,
               jwt::kAdminPermission,
               void,
               const StorageID &,
               const HealthReport &)
    API_METHOD(StorageDeclareSector,
               jwt::kAdminPermission,
               void,
               const StorageID &,
               const SectorId &,
               const SectorFileType &,
               bool)
    API_METHOD(StorageDropSector,
               jwt::kAdminPermission,
               void,
               const StorageID &,
               const SectorId &,
               const SectorFileType &)
    API_METHOD(StorageFindSector,
               jwt::kAdminPermission,
               std::vector<StorageInfo_>,
               const SectorId &,
               const SectorFileType &,
               boost::optional<SectorSize>)
    API_METHOD(StorageBestAlloc,
               jwt::kAdminPermission,
               std::vector<StorageInfo_>,
               const SectorFileType &,
               SectorSize,
               bool)

    API_METHOD(ReturnAddPiece,
               jwt::kAdminPermission,
               void,
               const CallId &,
               const PieceInfo &,
               const boost::optional<CallError> &)
    API_METHOD(ReturnSealPreCommit1,
               jwt::kAdminPermission,
               void,
               const CallId &,
               const PreCommit1Output &,
               const boost::optional<CallError> &)
    API_METHOD(ReturnSealPreCommit2,
               jwt::kAdminPermission,
               void,
               const CallId &,
               const SectorCids &,
               const boost::optional<CallError> &)
    API_METHOD(ReturnSealCommit1,
               jwt::kAdminPermission,
               void,
               const CallId &,
               const Commit1Output &,
               const boost::optional<CallError> &)
    API_METHOD(ReturnSealCommit2,
               jwt::kAdminPermission,
               void,
               const CallId &,
               const Proof &,
               const boost::optional<CallError> &)
    API_METHOD(ReturnFinalizeSector,
               jwt::kAdminPermission,
               void,
               const CallId &,
               const boost::optional<CallError> &)
    API_METHOD(ReturnMoveStorage,
               jwt::kAdminPermission,
               void,
               const CallId &,
               const boost::optional<CallError> &)
    API_METHOD(ReturnUnsealPiece,
               jwt::kAdminPermission,
               void,
               const CallId &,
               const boost::optional<CallError> &)
    API_METHOD(ReturnReadPiece,
               jwt::kAdminPermission,
               void,
               const CallId &,
               bool,
               const boost::optional<CallError> &)
    API_METHOD(ReturnFetch,
               jwt::kAdminPermission,
               void,
               const CallId &,
               const boost::optional<CallError> &)

    API_METHOD(WorkerConnect, jwt::kAdminPermission, void, const std::string &);
  };

  /**
   * Creates StorageMinerApi implementation
   * @return initialized StorageMinerApi
   */
  std::shared_ptr<StorageMinerApi> makeStorageApi(
      const std::shared_ptr<io_context> &io,
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
    f(a.SectorsList);
    f(a.SectorsStatus);
    f(a.StorageAttach);
    f(a.StorageInfo);
    f(a.StorageReportHealth);
    f(a.StorageDeclareSector);
    f(a.StorageDropSector);
    f(a.StorageFindSector);
    f(a.StorageBestAlloc);
    f(a.ReturnAddPiece);
    f(a.ReturnSealPreCommit1);
    f(a.ReturnSealPreCommit2);
    f(a.ReturnSealCommit1);
    f(a.ReturnSealCommit2);
    f(a.ReturnFinalizeSector);
    f(a.ReturnMoveStorage);
    f(a.ReturnUnsealPiece);
    f(a.ReturnReadPiece);
    f(a.ReturnFetch);
    f(a.WorkerConnect);
  }
}  // namespace fc::api
