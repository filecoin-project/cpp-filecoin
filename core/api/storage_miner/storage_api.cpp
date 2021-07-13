/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/storage_miner/storage_api.hpp"

#include "sector_storage/impl/remote_worker.hpp"

namespace fc::api {
  using sector_storage::RemoteWorker;

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
      const std::shared_ptr<RetrievalProvider> &retrieval_market_provider) {
    auto api = std::make_shared<StorageMinerApi>();

    api->ActorAddress = {[=]() { return miner->getAddress(); }};

    api->ActorSectorSize = [=](auto &addr) -> outcome::result<api::SectorSize> {
      OUTCOME_TRY(miner_info, full_node_api->StateMinerInfo(addr, {}));
      return miner_info.sector_size;
    };

    api->PledgeSector = [&]() -> outcome::result<void> {
      return miner->getSealing()->pledgeSector();
    };

    api->DealsImportData = [=](auto &proposal, auto &path) {
      return storage_market_provider->importDataForDeal(proposal, path);
    };

    api->MarketGetAsk = [=]() -> outcome::result<SignedStorageAsk> {
      return stored_ask->getAsk(actor);
    };

    api->MarketGetRetrievalAsk = [=]() -> outcome::result<RetrievalAsk> {
      return retrieval_market_provider->getAsk();
    };

    api->MarketSetAsk = [=](auto &price,
                            auto &verified_price,
                            auto duration,
                            auto min_piece_size,
                            auto max_piece_size) -> outcome::result<void> {
      return stored_ask->addAsk(
          {
              .price = price,
              .verified_price = verified_price,
              .min_piece_size = min_piece_size,
              .max_piece_size = max_piece_size,
              .miner = actor,
          },
          duration);
    };

    api->MarketSetRetrievalAsk = [=](auto &ask) -> outcome::result<void> {
      retrieval_market_provider->setAsk(ask);
      return outcome::success();
    };

    api->StorageAttach = [=](const StorageInfo_ &storage_info,
                             const FsStat &stat) {
      return sector_index->storageAttach(storage_info, stat);
    };

    api->StorageInfo = [=](const StorageID &storage_id) {
      return sector_index->getStorageInfo(storage_id);
    };

    api->StorageReportHealth = [=](const StorageID &storage_id,
                                   const HealthReport &report) {
      return sector_index->storageReportHealth(storage_id, report);
    };

    api->StorageDeclareSector = [=](const StorageID &storage_id,
                                    const SectorId &sector,
                                    const SectorFileType &file_type,
                                    bool primary) {
      return sector_index->storageDeclareSector(
          storage_id, sector, file_type, primary);
    };

    api->StorageDropSector = [=](const StorageID &storage_id,
                                 const SectorId &sector,
                                 const SectorFileType &file_type) {
      return sector_index->storageDropSector(storage_id, sector, file_type);
    };

    api->StorageFindSector =
        [=](const SectorId &sector,
            const SectorFileType &file_type,
            boost::optional<SectorSize> fetch_sector_size) {
          return sector_index->storageFindSector(
              sector, file_type, fetch_sector_size);
        };

    api->StorageBestAlloc = [=](const SectorFileType &allocate,
                                SectorSize sector_size,
                                bool sealing_mode) {
      return sector_index->storageBestAlloc(
          allocate, sector_size, sealing_mode);
    };

    api->ReturnAddPiece = [=](const CallId &call_id,
                              const boost::optional<PieceInfo> &piece_info,
                              const boost::optional<CallError> &call_error) {
      return sector_scheduler->returnAddPiece(call_id, piece_info, call_error);
    };

    api->ReturnSealPreCommit1 =
        [=](const CallId &call_id,
            const boost::optional<PreCommit1Output> &precommit1_output,
            const boost::optional<CallError> &call_error) {
          return sector_scheduler->returnSealPreCommit1(
              call_id, precommit1_output, call_error);
        };

    api->ReturnSealPreCommit2 =
        [=](const CallId &call_id,
            const boost::optional<SectorCids> &cids,
            const boost::optional<CallError> &call_error) {
          return sector_scheduler->returnSealPreCommit2(
              call_id, cids, call_error);
        };

    api->ReturnSealCommit1 =
        [=](const CallId &call_id,
            const boost::optional<Commit1Output> &commit1_out,
            const boost::optional<CallError> &call_error) {
          return sector_scheduler->returnSealCommit1(
              call_id, commit1_out, call_error);
        };

    api->ReturnSealCommit2 = [=](const CallId &call_id,
                                 const boost::optional<Proof> &proof,
                                 const boost::optional<CallError> &call_error) {
      return sector_scheduler->returnSealCommit2(call_id, proof, call_error);
    };

    api->ReturnFinalizeSector =
        [=](const CallId &call_id,
            const boost::optional<CallError> &call_error) {
          return sector_scheduler->returnFinalizeSector(call_id, call_error);
        };

    api->ReturnMoveStorage = [=](const CallId &call_id,
                                 const boost::optional<CallError> &call_error) {
      return sector_scheduler->returnMoveStorage(call_id, call_error);
    };

    api->ReturnUnsealPiece = [=](const CallId &call_id,
                                 const boost::optional<CallError> &call_error) {
      return sector_scheduler->returnUnsealPiece(call_id, call_error);
    };

    api->ReturnReadPiece = [=](const CallId &call_id,
                               const boost::optional<bool> &status,
                               const boost::optional<CallError> &call_error) {
      return sector_scheduler->returnReadPiece(call_id, status, call_error);
    };

    api->ReturnFetch = [=](const CallId &call_id,
                           const boost::optional<CallError> &call_error) {
      return sector_scheduler->returnFetch(call_id, call_error);
    };

    api->WorkerConnect =
        [=, self{api}](const std::string &address) -> outcome::result<void> {
      OUTCOME_TRY(maddress, libp2p::multi::Multiaddress::create(address));
      OUTCOME_TRY(worker,
                  RemoteWorker::connectRemoteWorker(*io, self, maddress));

      spdlog::info("Connected to a remote worker at {}", address);

      return sector_manager->addWorker(std::move(worker));
    };

    api->Version = [] {
      return api::VersionResult{"fuhon-miner", kMinerApiVersion, 0};
    };

    return api;
  }
}  // namespace fc::api
