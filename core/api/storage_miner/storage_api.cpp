/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/storage_miner/storage_api.hpp"

#include "api/storage_miner/return_api.hpp"
#include "miner/miner_version.hpp"
#include "sector_storage/impl/remote_worker.hpp"

namespace fc::api {
  using miner::kMinerVersion;
  using sector_storage::RemoteWorker;

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
      const std::shared_ptr<RetrievalProvider> &retrieval_market_provider) {
    auto api = std::make_shared<StorageMinerApi>();

    api->ActorAddress = [=]() { return miner->getAddress(); };

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

    api->MarketListIncompleteDeals =
        [=]() -> outcome::result<std::vector<MinerDeal>> {
      return storage_market_provider->getLocalDeals();
    };

    api->SectorsList = [=]() -> outcome::result<std::vector<SectorNumber>> {
      std::vector<SectorNumber> result;
      for (const auto &sector : miner->getSealing()->getListSectors()) {
        if (sector->state != mining::SealingState::kStateUnknown) {
          result.push_back(sector->sector_number);
        }
      }

      return result;
    };

    api->SectorsStatus =
        [=](SectorNumber id,
            bool show_onchain_info) -> outcome::result<ApiSectorInfo> {
      OUTCOME_TRY(sector_info, miner->getSealing()->getSectorInfo(id));

      const auto &pieces{sector_info->pieces};
      std::vector<DealId> deals;
      deals.reserve(sector_info->pieces.size());

      for (const auto &piece : pieces) {
        deals.push_back(piece.deal_info ? piece.deal_info->deal_id : 0);
      }
      ApiSectorInfo api_sector_info{
          sector_info->state,
          id,
          sector_info->sector_type,
          sector_info->comm_d,
          sector_info->comm_r,
          sector_info->proof,
          std::move(deals),
          pieces,
          sector_info->ticket,
          sector_info->seed,
          sector_info->precommit_message,
          sector_info->message,
          sector_info->invalid_proofs,
          miner->getSealing()->isMarkedForUpgrade(id),
      };
      if (not show_onchain_info) {
        return api_sector_info;
      }
      OUTCOME_TRY(chain_info,
                  full_node_api->StateSectorGetInfo(
                      miner->getAddress(), id, TipsetKey{}));
      if (!chain_info.has_value()) {
        return api_sector_info;
      }
      api_sector_info.seal_proof = chain_info->seal_proof;
      api_sector_info.activation = chain_info->activation_epoch;
      api_sector_info.expiration = chain_info->expiration;
      api_sector_info.deal_weight = chain_info->deal_weight;
      api_sector_info.verified_deal_weight = chain_info->verified_deal_weight;
      api_sector_info.initial_pledge = chain_info->init_pledge;
      auto maybe_expiration_info = full_node_api->StateSectorExpiration(
          miner->getAddress(), id, TipsetKey{});
      if (maybe_expiration_info.has_value()) {
        api_sector_info.on_time = maybe_expiration_info.value().on_time;
        api_sector_info.early = maybe_expiration_info.value().early;
      }
      return api_sector_info;
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

    makeReturnApi(api, sector_scheduler);

    api->WorkerConnect =
        [=, self{api}](const std::string &address) -> outcome::result<void> {
      OUTCOME_TRY(maddress, libp2p::multi::Multiaddress::create(address));
      OUTCOME_TRY(worker,
                  RemoteWorker::connectRemoteWorker(*io, self, maddress));

      spdlog::info("Connected to a remote worker at {}", address);

      return sector_manager->addWorker(std::move(worker));
    };

    api->Version = [] {
      return api::VersionResult{kMinerVersion, kMinerApiVersion, 0};
    };

    return api;
  }
}  // namespace fc::api
