/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/manager_impl.hpp"

#include <pwd.h>
#include <boost/filesystem.hpp>
#include <unordered_set>
#include "sector_storage/impl/allocate_selector.hpp"
#include "sector_storage/impl/existing_selector.hpp"
#include "sector_storage/impl/local_worker.hpp"
#include "sector_storage/impl/task_selector.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fs = boost::filesystem;
using fc::primitives::sector_file::SectorFileType;
using fc::primitives::sector_file::sectorName;

namespace {
  fc::sector_storage::WorkerAction schedFetch(const SectorId &sector,
                                              SectorFileType file_type,
                                              bool sealing) {
    return [=](const std::shared_ptr<fc::sector_storage::Worker> &worker)
               -> fc::outcome::result<void> {
      return worker->fetch(sector, file_type, sealing);
    };
  }

  fc::outcome::result<void> schedNothing(
      const std::shared_ptr<fc::sector_storage::Worker> &worker) {
    return fc::outcome::success();
  }

  void addCachePathsForSectorSize(
      std::unordered_map<std::string, uint64_t> &check,
      const std::string &cache_dir,
      SectorSize ssize,
      const fc::common::Logger &logger) {
    switch (ssize) {
      case SectorSize(2) << 10:
      case SectorSize(8) << 20:
      case SectorSize(512) << 20:
        check[(fs::path(cache_dir) / "sc-02-data-tree-r-last.dat").string()] =
            0;
        break;
      case SectorSize(32) << 30:
        for (int i = 0; i < 8; i++) {
          check[(fs::path(cache_dir)
                 / ("sc-02-data-tree-r-last-" + std::to_string(i) + ".dat"))
                    .string()] = 0;
        }
        break;
      case SectorSize(64) << 30:
        for (int i = 0; i < 16; i++) {
          check[(fs::path(cache_dir)
                 / ("sc-02-data-tree-r-last-" + std::to_string(i) + ".dat"))
                    .string()] = 0;
        }
        break;
      default:
        logger->warn("not checking cache files of {} sectors for faults",
                     ssize);
        break;
    }
  }

  fc::outcome::result<std::string> expandPath(const std::string &path) {
    if (path.empty() || path[0] != '~') return path;

    std::string home_dir;
    const char *home = getenv("HOME");
    if (home == nullptr) {
      struct passwd *pwd = getpwuid(getuid());
      if (pwd) {
        home_dir = pwd->pw_dir;
      } else {
        return fc::sector_storage::ManagerErrors::kCannotGetHomeDir;
      }
    } else {
      home_dir = home;
    }

    return (fs::path(home_dir) / path.substr(1, path.size() - 1)).string();
  }
}  // namespace

namespace fc::sector_storage {

  outcome::result<std::vector<SectorId>> ManagerImpl::checkProvable(
      RegisteredProof seal_proof_type, gsl::span<const SectorId> sectors) {
    std::vector<SectorId> bad{};

    OUTCOME_TRY(ssize, primitives::sector::getSectorSize(seal_proof_type));

    for (const auto &sector : sectors) {
      auto locked = index_->storageTryLock(
          sector,
          static_cast<SectorFileType>(SectorFileType::FTSealed
                                      | SectorFileType::FTCache),
          SectorFileType::FTNone);

      if (!locked) {
        logger_->warn("can't acquire read lock for {} sector",
                      sectorName(sector));
        bad.push_back(sector);
        continue;
      }

      auto maybe_response = local_store_->acquireSector(
          sector,
          seal_proof_type,
          static_cast<SectorFileType>(SectorFileType::FTSealed
                                      | SectorFileType::FTCache),
          SectorFileType::FTNone,
          false);

      if (maybe_response.has_error()) {
        if (maybe_response
            == outcome::failure(
                stores::StoreErrors::kNotFoundRequestedSectorType)) {
          logger_->warn("cache an/or sealed paths not found for {} sector",
                        sectorName(sector));
          bad.push_back(sector);
          continue;
        }
        return maybe_response.error();
      }

      std::unordered_map<std::string, uint64_t> to_check = {
          {maybe_response.value().paths.sealed, 1},
          {(fs::path(maybe_response.value().paths.cache) / "t_aux").string(),
           0},
          {(fs::path(maybe_response.value().paths.cache) / "p_aux").string(),
           0},
      };

      addCachePathsForSectorSize(
          to_check, maybe_response.value().paths.cache, ssize, logger_);

      for (const auto &[path, size] : to_check) {
        if (!fs::exists(path)) {
          logger_->warn(
              "{} doesnt exist for {} sector", path, sectorName(sector));
          bad.push_back(sector);
          break;
        }

        if (size != 0) {
          boost::system::error_code ec;
          size_t actual_size = fs::file_size(path, ec);
          if (ec.failed()) {
            logger_->warn("sector {}. Can't get size for {}: {}",
                          sectorName(sector),
                          path,
                          ec.message());
            bad.push_back(sector);
            break;
          }

          if (actual_size != ssize * size) {
            logger_->warn(
                "sector {}. Actual and declared sizes do not match for {}",
                sectorName(sector),
                path);
            bad.push_back(sector);
            break;
          }
        }
      }
    }

    return std::move(bad);
  }

  SectorSize ManagerImpl::getSectorSize() {
    auto maybe_size = primitives::sector::getSectorSize(seal_proof_type_);
    if (maybe_size.has_value()) {
      return maybe_size.value();
    }
    return 0;
  }

  outcome::result<void> ManagerImpl::ReadPiece(proofs::PieceData output,
                                               const SectorId &sector,
                                               UnpaddedByteIndex offset,
                                               const UnpaddedPieceSize &size,
                                               const SealRandomness &randomness,
                                               const CID &cid) {
    OUTCOME_TRY(lock,
                index_->storageLock(
                    sector,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTUnsealed));

    {
      OUTCOME_TRY(
          best,
          index_->storageFindSector(sector, SectorFileType::FTUnsealed, false));

      std::shared_ptr<WorkerSelector> selector;
      if (best.empty()) {
        selector = std::make_unique<AllocateSelector>(
            index_, SectorFileType::FTUnsealed, true);
      } else {
        OUTCOME_TRYA(selector,
                     ExistingSelector::newExistingSelector(
                         index_, sector, SectorFileType::FTUnsealed, false));
      }

      // TODO: Optimization: don't send unseal to a worker if the requested
      // range is already unsealed

      WorkerAction unseal_fetch =
          [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
        SectorFileType unsealed =
            best.empty() ? SectorFileType::FTNone : SectorFileType::FTUnsealed;

        OUTCOME_TRY(worker->fetch(
            sector,
            static_cast<SectorFileType>(SectorFileType::FTSealed
                                        | SectorFileType::FTCache | unsealed),
            true));

        return outcome::success();
      };

      OUTCOME_TRY(scheduler_->schedule(
          sector,
          primitives::kTTUnseal,
          selector,
          unseal_fetch,
          [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
            return worker->unsealPiece(sector, offset, size, randomness, cid);
          }));
    }

    std::shared_ptr<WorkerSelector> selector;
    OUTCOME_TRYA(selector,
                 ExistingSelector::newExistingSelector(
                     index_, sector, SectorFileType::FTUnsealed, false));

    return scheduler_->schedule(
        sector,
        primitives::kTTReadUnsealed,
        selector,
        schedFetch(sector, SectorFileType::FTUnsealed, true),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          return worker->readPiece(std::move(output), sector, offset, size);
        });
  }

  outcome::result<std::vector<PoStProof>> ManagerImpl::generateWinningPoSt(
      ActorId miner_id,
      gsl::span<const SectorInfo> sector_info,
      PoStRandomness randomness) {
    randomness[31] = 0;

    OUTCOME_TRY(res,
                publicSectorToPrivate(
                    miner_id,
                    sector_info,
                    {},
                    primitives::sector::getRegisteredWinningPoStProof));

    if (!res.skipped.empty()) {
      std::string skipped_sectors = sectorName(res.skipped[0]);
      for (size_t i = 1; i < res.skipped.size(); i++) {
        skipped_sectors += ", " + sectorName(res.skipped[i]);
      }
      logger_->error("skipped sectors: " + skipped_sectors);
      return ManagerErrors::kSomeSectorSkipped;
    }

    return proofs::Proofs::generateWinningPoSt(
        miner_id, res.private_info, randomness);
  }

  outcome::result<Prover::WindowPoStResponse> ManagerImpl::generateWindowPoSt(
      ActorId miner_id,
      gsl::span<const SectorInfo> sector_info,
      PoStRandomness randomness) {
    // TODO: maybe some manipulation with randomness

    Prover::WindowPoStResponse response{};

    OUTCOME_TRY(res,
                publicSectorToPrivate(
                    miner_id,
                    sector_info,
                    {},
                    primitives::sector::getRegisteredWindowPoStProof));

    OUTCOME_TRYA(response.proof,
                 proofs::Proofs::generateWindowPoSt(
                     miner_id, res.private_info, randomness));

    response.skipped = std::move(res.skipped);

    return std::move(response);
  }

  outcome::result<PreCommit1Output> ManagerImpl::sealPreCommit1(
      const SectorId &sector,
      const SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    OUTCOME_TRY(lock,
                index_->storageLock(
                    sector,
                    SectorFileType::FTUnsealed,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache)));

    // TODO: also consider where the unsealed data sits

    auto selector = std::make_unique<AllocateSelector>(
        index_,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        true);

    PreCommit1Output out;

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTPreCommit1,
        std::move(selector),
        schedFetch(sector, SectorFileType::FTUnsealed, true),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          OUTCOME_TRYA(out, worker->sealPreCommit1(sector, ticket, pieces));
          return outcome::success();
        }));

    return std::move(out);
  }

  outcome::result<SectorCids> ManagerImpl::sealPreCommit2(
      const SectorId &sector, const PreCommit1Output &pre_commit_1_output) {
    OUTCOME_TRY(lock,
                index_->storageLock(
                    sector, SectorFileType::FTSealed, SectorFileType::FTCache));

    OUTCOME_TRY(selector,
                ExistingSelector::newExistingSelector(
                    index_,
                    sector,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    true));

    SectorCids out;

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTPreCommit2,
        std::move(selector),
        schedFetch(sector,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache),
                   true),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          OUTCOME_TRYA(out, worker->sealPreCommit2(sector, pre_commit_1_output))
          return outcome::success();
        }));

    return std::move(out);
  }

  outcome::result<Commit1Output> ManagerImpl::sealCommit1(
      const SectorId &sector,
      const SealRandomness &ticket,
      const InteractiveRandomness &seed,
      gsl::span<const PieceInfo> pieces,
      const SectorCids &cids) {
    OUTCOME_TRY(lock,
                index_->storageLock(
                    sector, SectorFileType::FTSealed, SectorFileType::FTCache));

    OUTCOME_TRY(selector,
                ExistingSelector::newExistingSelector(
                    index_,
                    sector,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    false));

    Commit1Output out;

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTCommit1,
        std::move(selector),
        schedFetch(sector,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache),
                   true),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          OUTCOME_TRYA(out,
                       worker->sealCommit1(sector, ticket, seed, pieces, cids))
          return outcome::success();
        }));

    return std::move(out);
  }

  outcome::result<Proof> ManagerImpl::sealCommit2(
      const SectorId &sector, const Commit1Output &commit_1_output) {
    std::unique_ptr<TaskSelector> selector = std::make_unique<TaskSelector>();

    Proof out;

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTCommit2,
        std::move(selector),
        schedNothing,
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          OUTCOME_TRYA(out, worker->sealCommit2(sector, commit_1_output))
          return outcome::success();
        }));

    return std::move(out);
  }

  outcome::result<void> ManagerImpl::finalizeSector(const SectorId &sector) {
    OUTCOME_TRY(lock,
                index_->storageLock(
                    sector,
                    SectorFileType::FTNone,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTUnsealed
                                                | SectorFileType::FTCache)));

    auto unsealed = SectorFileType::FTUnsealed;
    {
      OUTCOME_TRY(
          unsealed_stores,
          index_->storageFindSector(sector, SectorFileType::FTUnsealed, false));

      if (unsealed_stores.empty()) {
        unsealed = SectorFileType::FTNone;
      }
    }

    std::shared_ptr<WorkerSelector> selector;

    OUTCOME_TRYA(selector,
                 ExistingSelector::newExistingSelector(
                     index_,
                     sector,
                     static_cast<SectorFileType>(SectorFileType::FTSealed
                                                 | SectorFileType::FTCache),
                     false));

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTFinalize,
        selector,
        schedFetch(
            sector,
            static_cast<SectorFileType>(SectorFileType::FTSealed
                                        | SectorFileType::FTCache | unsealed),
            true),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          return worker->finalizeSector(sector);
        }));

    auto fetch_selector = std::make_shared<AllocateSelector>(
        index_,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        false);

    return scheduler_->schedule(
        sector,
        primitives::kTTFetch,
        fetch_selector,
        schedFetch(sector,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache),
                   false),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          return worker->moveStorage(sector);
        });
  }

  outcome::result<void> ManagerImpl::remove(const SectorId &sector) {
    OUTCOME_TRY(lock,
                index_->storageLock(
                    sector,
                    SectorFileType::FTNone,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTUnsealed
                                                | SectorFileType::FTCache)));

    auto unsealed = SectorFileType::FTUnsealed;
    {
      OUTCOME_TRY(
          unsealed_stores,
          index_->storageFindSector(sector, SectorFileType::FTUnsealed, false));

      if (unsealed_stores.empty()) {
        unsealed = SectorFileType::FTNone;
      }
    }

    std::shared_ptr<WorkerSelector> selector;

    OUTCOME_TRYA(selector,
                 ExistingSelector::newExistingSelector(
                     index_,
                     sector,
                     static_cast<SectorFileType>(SectorFileType::FTSealed
                                                 | SectorFileType::FTCache),
                     false));

    return scheduler_->schedule(
        sector,
        primitives::kTTFinalize,
        selector,
        schedFetch(
            sector,
            static_cast<SectorFileType>(SectorFileType::FTSealed
                                        | SectorFileType::FTCache | unsealed),
            false),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          return worker->remove(sector);
        });
  }

  outcome::result<PieceInfo> ManagerImpl::addPiece(
      const SectorId &sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      const proofs::PieceData &piece_data) {
    OUTCOME_TRY(
        lock,
        index_->storageLock(
            sector, SectorFileType::FTNone, SectorFileType::FTUnsealed));

    std::shared_ptr<WorkerSelector> selector;
    if (piece_sizes.empty()) {
      selector = std::make_unique<AllocateSelector>(
          index_, SectorFileType::FTUnsealed, true);
    } else {
      OUTCOME_TRYA(selector,
                   ExistingSelector::newExistingSelector(
                       index_, sector, SectorFileType::FTUnsealed, false));
    }

    PieceInfo out;

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTAddPiece,
        selector,
        schedNothing,
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          OUTCOME_TRYA(out,
                       worker->addPiece(
                           sector, piece_sizes, new_piece_size, piece_data));
          return outcome::success();
        }));

    return std::move(out);
  }

  outcome::result<void> ManagerImpl::addLocalStorage(const std::string &path) {
    OUTCOME_TRY(ePath, expandPath(path));

    return local_store_->openPath(ePath);
  }

  outcome::result<void> ManagerImpl::addWorker(std::shared_ptr<Worker> worker) {
    OUTCOME_TRY(info, worker->getInfo());

    auto worker_handler = std::make_unique<WorkerHandle>();

    worker_handler->worker = std::move(worker);
    worker_handler->info = std::move(info);

    scheduler_->newWorker(std::move(worker_handler));

    return outcome::success();
  }

  outcome::result<std::unordered_map<StorageID, std::string>>
  ManagerImpl::getLocalStorages() {
    OUTCOME_TRY(paths, local_store_->getAccessiblePaths());

    std::unordered_map<StorageID, std::string> out{};

    for (const auto &path : paths) {
      out[path.id] = path.local_path;
    }

    return std::move(out);
  }

  outcome::result<FsStat> ManagerImpl::getFsStat(StorageID storage_id) {
    return local_store_->getFsStat(storage_id);
  }

  outcome::result<ManagerImpl::PubToPrivateResponse>
  ManagerImpl::publicSectorToPrivate(
      ActorId miner,
      gsl::span<const SectorInfo> sector_info,
      gsl::span<const SectorNumber> faults,
      const std::function<outcome::result<RegisteredProof>(RegisteredProof)>
          &to_post_transform) {
    PubToPrivateResponse result;

    std::unordered_set<SectorNumber> faults_set;
    for (const auto &fault : faults) {
      faults_set.insert(fault);
    }

    std::vector<proofs::PrivateSectorInfo> out{};
    for (const auto &sector : sector_info) {
      if (faults_set.find(sector.sector) != faults_set.end()) {
        continue;
      }

      SectorId sector_id{
          .miner = miner,
          .sector = sector.sector,
      };

      auto res = local_store_->acquireSector(
          sector_id,
          seal_proof_type_,
          static_cast<SectorFileType>(SectorFileType::FTCache
                                      | SectorFileType::FTSealed),
          SectorFileType::FTNone,
          false);
      if (res.has_error()) {
        logger_->warn("failed to acquire sector {}", sectorName(sector_id));
        result.skipped.push_back(sector_id);
        continue;
      }

      OUTCOME_TRY(post_proof_type, to_post_transform(sector.registered_proof));

      out.push_back(proofs::PrivateSectorInfo{
          .info = sector,
          .cache_dir_path = res.value().paths.cache,
          .post_proof_type = post_proof_type,
          .sealed_sector_path = res.value().paths.sealed,
      });
    }

    result.private_info = proofs::Proofs::newSortedPrivateSectorInfo(out);

    return std::move(result);
  }

  outcome::result<std::unique_ptr<Manager>> ManagerImpl::newManager(
      const std::shared_ptr<stores::RemoteStore> &remote,
      const std::shared_ptr<Scheduler> &scheduler,
      const SealerConfig &config) {
    struct make_unique_enabler : public ManagerImpl {
      make_unique_enabler(std::shared_ptr<stores::SectorIndex> sector_index,
                          RegisteredProof seal_proof_type,
                          std::shared_ptr<stores::LocalStorage> local_storage,
                          std::shared_ptr<stores::LocalStore> local_store,
                          std::shared_ptr<stores::RemoteStore> store,
                          std::shared_ptr<Scheduler> scheduler)
          : ManagerImpl{std::move(sector_index),
                        seal_proof_type,
                        std::move(local_storage),
                        std::move(local_store),
                        std::move(store),
                        std::move(scheduler)} {};
    };

    auto proof_type = scheduler->getSealProofType();
    auto local_store = remote->getLocalStore();
    auto local_storage = local_store->getLocalStorage();
    auto sector_index = remote->getSectorIndex();
    std::unique_ptr<ManagerImpl> manager =
        std::make_unique<make_unique_enabler>(sector_index,
                                              proof_type,
                                              local_storage,
                                              local_store,
                                              remote,
                                              scheduler);

    std::set<TaskType> local_tasks{
        primitives::kTTAddPiece,
        primitives::kTTCommit1,
        primitives::kTTFinalize,
        primitives::kTTFetch,
        primitives::kTTReadUnsealed,

    };

    if (config.allow_precommit_1) {
      local_tasks.insert(primitives::kTTPreCommit1);
    }

    if (config.allow_precommit_2) {
      local_tasks.insert(primitives::kTTPreCommit2);
    }

    if (config.allow_commit) {
      local_tasks.insert(primitives::kTTCommit2);
    }

    if (config.allow_unseal) {
      local_tasks.insert(primitives::kTTUnseal);
    }

    std::unique_ptr<Worker> worker = std::make_unique<LocalWorker>(
        WorkerConfig{
            .hostname = "",
            .seal_proof_type = proof_type,
            .task_types = std::move(local_tasks),
        },
        std::move(remote));

    OUTCOME_TRY(manager->addWorker(std::move(worker)));
    return std::move(manager);
  }

  ManagerImpl::ManagerImpl(std::shared_ptr<stores::SectorIndex> sector_index,
                           RegisteredProof seal_proof_type,
                           std::shared_ptr<stores::LocalStorage> local_storage,
                           std::shared_ptr<stores::LocalStore> local_store,
                           std::shared_ptr<stores::RemoteStore> store,
                           std::shared_ptr<Scheduler> scheduler)
      : index_(std::move(sector_index)),
        seal_proof_type_(seal_proof_type),
        local_storage_(std::move(local_storage)),
        local_store_(std::move(local_store)),
        remote_store_(std::move(store)),
        scheduler_(std::move(scheduler)),
        logger_(common::createLogger("manager")) {}
}  // namespace fc::sector_storage

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage, ManagerErrors, e) {
  using fc::sector_storage::ManagerErrors;
  switch (e) {
    case (ManagerErrors::kCannotGetHomeDir):
      return "Manager: cannot get HOME dir to expand path";
    case (ManagerErrors::kSomeSectorSkipped):
      return "Manager: some of sectors was skipped during generating of "
             "winning PoSt";
    default:
      return "Manager: unknown error";
  }
}
