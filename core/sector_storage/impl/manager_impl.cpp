/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/manager_impl.hpp"

#include <pwd.h>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/filesystem.hpp>
#include <regex>
#include <unordered_set>
#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "common/tarutil.hpp"
#include "sector_storage/impl/allocate_selector.hpp"
#include "sector_storage/impl/existing_selector.hpp"
#include "sector_storage/impl/local_worker.hpp"
#include "sector_storage/impl/task_selector.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fc::sector_storage {
  using fc::primitives::sector_file::SectorFileType;
  using fc::primitives::sector_file::sectorName;
  namespace fs = boost::filesystem;

  fc::sector_storage::WorkerAction schedFetch(const SectorId &sector,
                                              SectorFileType file_type,
                                              PathType path_type,
                                              AcquireMode mode) {
    return [=](const std::shared_ptr<fc::sector_storage::Worker> &worker)
               -> fc::outcome::result<void> {
      return worker->fetch(sector, file_type, path_type, mode);
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

  outcome::result<std::vector<SectorId>> ManagerImpl::checkProvable(
      RegisteredSealProof seal_proof_type, gsl::span<const SectorId> sectors) {
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
          PathType::kStorage,
          AcquireMode::kMove);

      if (maybe_response.has_error()) {
        if (maybe_response
            == outcome::failure(stores::StoreError::kNotFoundSector)) {
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

  outcome::result<void> ManagerImpl::readPiece(proofs::PieceData output,
                                               const SectorId &sector,
                                               UnpaddedByteIndex offset,
                                               const UnpaddedPieceSize &size,
                                               const SealRandomness &randomness,
                                               const CID &cid) {
    OUTCOME_TRY(index_->storageLock(
        sector,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        SectorFileType::FTUnsealed));

    {
      OUTCOME_TRY(best,
                  index_->storageFindSector(
                      sector, SectorFileType::FTUnsealed, boost::none));

      std::shared_ptr<WorkerSelector> selector;
      if (best.empty()) {
        selector = std::make_unique<AllocateSelector>(
            index_, SectorFileType::FTUnsealed, PathType::kSealing);
      } else {
        selector = std::make_shared<ExistingSelector>(
            index_, sector, SectorFileType::FTUnsealed, false);
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
            PathType::kSealing,
            AcquireMode::kMove));

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
    selector = std::make_shared<ExistingSelector>(
        index_, sector, SectorFileType::FTUnsealed, false);

    bool is_read_success = false;

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTReadUnsealed,
        selector,
        schedFetch(sector,
                   SectorFileType::FTUnsealed,
                   PathType::kSealing,
                   AcquireMode::kMove),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          OUTCOME_TRYA(
              is_read_success,
              worker->readPiece(std::move(output), sector, offset, size));
          return outcome::success();
        }));

    if (is_read_success) {
      return outcome::success();
    }

    return ManagerErrors::kCannotReadData;
  }

  outcome::result<std::vector<PoStProof>> ManagerImpl::generateWinningPoSt(
      ActorId miner_id,
      gsl::span<const SectorInfo> sector_info,
      PoStRandomness randomness) {
    randomness[31] &= 0x3f;

    OUTCOME_TRY(res,
                publicSectorToPrivate(
                    miner_id,
                    sector_info,
                    primitives::sector::getRegisteredWinningPoStProof));

    if (!res.skipped.empty()) {
      std::string skipped_sectors = sectorName(res.skipped[0]);
      for (size_t i = 1; i < res.skipped.size(); i++) {
        skipped_sectors += ", " + sectorName(res.skipped[i]);
      }
      logger_->error("skipped sectors: " + skipped_sectors);
      return ManagerErrors::kSomeSectorSkipped;
    }

    return proofs_->generateWinningPoSt(miner_id, res.private_info, randomness);
  }

  outcome::result<Prover::WindowPoStResponse> ManagerImpl::generateWindowPoSt(
      ActorId miner_id,
      gsl::span<const SectorInfo> sector_info,
      PoStRandomness randomness) {
    randomness[31] &= 0x3f;

    Prover::WindowPoStResponse response{};

    OUTCOME_TRY(res,
                publicSectorToPrivate(
                    miner_id,
                    sector_info,
                    primitives::sector::getRegisteredWindowPoStProof));

    OUTCOME_TRYA(
        response.proof,
        proofs_->generateWindowPoSt(miner_id, res.private_info, randomness));

    response.skipped = std::move(res.skipped);

    return std::move(response);
  }

  outcome::result<PreCommit1Output> ManagerImpl::sealPreCommit1(
      const SectorId &sector,
      const SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces,
      uint64_t priority) {
    OUTCOME_TRY(index_->storageLock(
        sector,
        SectorFileType::FTUnsealed,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache)));

    // TODO: also consider where the unsealed data sits

    auto selector = std::make_unique<AllocateSelector>(
        index_,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        PathType::kSealing);

    PreCommit1Output out;

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTPreCommit1,
        std::move(selector),
        schedFetch(sector,
                   SectorFileType::FTUnsealed,
                   PathType::kSealing,
                   AcquireMode::kMove),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          OUTCOME_TRYA(out, worker->sealPreCommit1(sector, ticket, pieces));
          return outcome::success();
        },
        priority));

    return std::move(out);
  }

  outcome::result<SectorCids> ManagerImpl::sealPreCommit2(
      const SectorId &sector,
      const PreCommit1Output &pre_commit_1_output,
      uint64_t priority) {
    OUTCOME_TRY(index_->storageLock(
        sector, SectorFileType::FTSealed, SectorFileType::FTCache));

    auto selector = std::make_shared<ExistingSelector>(
        index_,
        sector,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        true);

    SectorCids out;

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTPreCommit2,
        std::move(selector),
        schedFetch(sector,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache),
                   PathType::kSealing,
                   AcquireMode::kMove),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          OUTCOME_TRYA(out, worker->sealPreCommit2(sector, pre_commit_1_output))
          return outcome::success();
        },
        priority));

    return std::move(out);
  }

  outcome::result<Commit1Output> ManagerImpl::sealCommit1(
      const SectorId &sector,
      const SealRandomness &ticket,
      const InteractiveRandomness &seed,
      gsl::span<const PieceInfo> pieces,
      const SectorCids &cids,
      uint64_t priority) {
    OUTCOME_TRY(index_->storageLock(
        sector, SectorFileType::FTSealed, SectorFileType::FTCache));

    auto selector = std::make_shared<ExistingSelector>(
        index_,
        sector,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        false);

    Commit1Output out;

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTCommit1,
        std::move(selector),
        schedFetch(sector,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache),
                   PathType::kSealing,
                   AcquireMode::kMove),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          OUTCOME_TRYA(out,
                       worker->sealCommit1(sector, ticket, seed, pieces, cids))
          return outcome::success();
        },
        priority));

    return std::move(out);
  }

  outcome::result<Proof> ManagerImpl::sealCommit2(
      const SectorId &sector,
      const Commit1Output &commit_1_output,
      uint64_t priority) {
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
        },
        priority));

    return std::move(out);
  }

  outcome::result<void> ManagerImpl::finalizeSector(
      const SectorId &sector,
      const gsl::span<const Range> &keep_unsealed,
      uint64_t priority) {
    OUTCOME_TRY(index_->storageLock(
        sector,
        SectorFileType::FTNone,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTUnsealed
                                    | SectorFileType::FTCache)));

    auto unsealed = SectorFileType::FTUnsealed;
    {
      OUTCOME_TRY(unsealed_stores,
                  index_->storageFindSector(
                      sector, SectorFileType::FTUnsealed, boost::none));

      if (unsealed_stores.empty()) {
        unsealed = SectorFileType::FTNone;
      }
    }

    std::shared_ptr<WorkerSelector> selector;

    selector = std::make_shared<ExistingSelector>(
        index_,
        sector,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        false);

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTFinalize,
        selector,
        schedFetch(
            sector,
            static_cast<SectorFileType>(SectorFileType::FTSealed
                                        | SectorFileType::FTCache | unsealed),
            PathType::kSealing,
            AcquireMode::kMove),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          return worker->finalizeSector(sector, keep_unsealed);
        },
        priority));

    auto fetch_selector = std::make_shared<AllocateSelector>(
        index_,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        PathType::kStorage);

    return scheduler_->schedule(
        sector,
        primitives::kTTFetch,
        fetch_selector,
        schedFetch(sector,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache),
                   PathType::kStorage,
                   AcquireMode::kMove),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          return worker->moveStorage(sector);
        },
        priority);
  }

  outcome::result<void> ManagerImpl::remove(const SectorId &sector) {
    OUTCOME_TRY(index_->storageLock(
        sector,
        SectorFileType::FTNone,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTUnsealed
                                    | SectorFileType::FTCache)));

    auto unsealed = SectorFileType::FTUnsealed;
    {
      OUTCOME_TRY(unsealed_stores,
                  index_->storageFindSector(
                      sector, SectorFileType::FTUnsealed, boost::none));

      if (unsealed_stores.empty()) {
        unsealed = SectorFileType::FTNone;
      }
    }

    std::shared_ptr<WorkerSelector> selector =
        std::make_shared<ExistingSelector>(
            index_,
            sector,
            static_cast<SectorFileType>(SectorFileType::FTSealed
                                        | SectorFileType::FTCache),
            false);

    return scheduler_->schedule(
        sector,
        primitives::kTTFinalize,
        selector,
        schedFetch(
            sector,
            static_cast<SectorFileType>(SectorFileType::FTSealed
                                        | SectorFileType::FTCache | unsealed),
            PathType::kStorage,
            AcquireMode::kMove),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<void> {
          return worker->remove(sector);
        });
  }

  outcome::result<PieceInfo> ManagerImpl::addPiece(
      const SectorId &sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      const proofs::PieceData &piece_data,
      uint64_t priority) {
    OUTCOME_TRY(index_->storageLock(
        sector, SectorFileType::FTNone, SectorFileType::FTUnsealed));

    std::shared_ptr<WorkerSelector> selector;
    if (piece_sizes.empty()) {
      selector = std::make_unique<AllocateSelector>(
          index_, SectorFileType::FTUnsealed, PathType::kSealing);
    } else {
      selector = std::make_shared<ExistingSelector>(
          index_, sector, SectorFileType::FTUnsealed, false);
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
        },
        priority));

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
      const std::function<outcome::result<RegisteredPoStProof>(
          RegisteredSealProof)> &to_post_transform) {
    PubToPrivateResponse result;

    std::vector<proofs::PrivateSectorInfo> out{};
    for (const auto &sector : sector_info) {
      SectorId sector_id{
          .miner = miner,
          .sector = sector.sector,
      };

      auto res =
          acquireSector(sector_id,
                        static_cast<SectorFileType>(SectorFileType::FTCache
                                                    | SectorFileType::FTSealed),
                        SectorFileType::FTNone,
                        PathType::kStorage);
      if (res.has_error()) {
        logger_->warn("failed to acquire sector {}", sectorName(sector_id));
        result.skipped.push_back(sector_id);
        continue;
      }

      OUTCOME_TRY(post_proof_type, to_post_transform(sector.registered_proof));

      result.locks.push_back(std::move(res.value().lock));

      out.push_back(proofs::PrivateSectorInfo{
          .info = sector,
          .cache_dir_path = res.value().paths.cache,
          .post_proof_type = post_proof_type,
          .sealed_sector_path = res.value().paths.sealed,
      });
    }

    result.private_info = proofs::newSortedPrivateSectorInfo(out);

    return std::move(result);
  }

  outcome::result<std::shared_ptr<Manager>> ManagerImpl::newManager(
      const std::shared_ptr<stores::RemoteStore> &remote,
      const std::shared_ptr<Scheduler> &scheduler,
      const SealerConfig &config,
      const std::shared_ptr<proofs::ProofEngine> &proofs) {
    struct make_unique_enabler : public ManagerImpl {
      make_unique_enabler(std::shared_ptr<stores::SectorIndex> sector_index,
                          RegisteredSealProof seal_proof_type,
                          std::shared_ptr<stores::LocalStorage> local_storage,
                          std::shared_ptr<stores::LocalStore> local_store,
                          std::shared_ptr<stores::RemoteStore> store,
                          std::shared_ptr<Scheduler> scheduler,
                          std::shared_ptr<proofs::ProofEngine> proofs)
          : ManagerImpl{std::move(sector_index),
                        seal_proof_type,
                        std::move(local_storage),
                        std::move(local_store),
                        std::move(store),
                        std::move(scheduler),
                        std::move(proofs)} {};
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
                                              scheduler,
                                              proofs);

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
            .seal_proof_type = proof_type,
            .task_types = std::move(local_tasks),
        },
        remote,
        proofs);

    OUTCOME_TRY(manager->addWorker(std::move(worker)));
    return std::move(manager);
  }

  ManagerImpl::ManagerImpl(std::shared_ptr<stores::SectorIndex> sector_index,
                           RegisteredSealProof seal_proof_type,
                           std::shared_ptr<stores::LocalStorage> local_storage,
                           std::shared_ptr<stores::LocalStore> local_store,
                           std::shared_ptr<stores::RemoteStore> store,
                           std::shared_ptr<Scheduler> scheduler,
                           std::shared_ptr<proofs::ProofEngine> proofs)
      : index_(std::move(sector_index)),
        seal_proof_type_(seal_proof_type),
        local_storage_(std::move(local_storage)),
        local_store_(std::move(local_store)),
        remote_store_(std::move(store)),
        scheduler_(std::move(scheduler)),
        logger_(common::createLogger("manager")),
        proofs_(std::move(proofs)) {}

  outcome::result<ManagerImpl::Response> ManagerImpl::acquireSector(
      SectorId sector_id,
      SectorFileType exisitng,
      SectorFileType allocate,
      PathType path) {
    if (allocate != SectorFileType::FTNone) {
      return ManagerErrors::kReadOnly;
    }

    auto locked =
        index_->storageTryLock(sector_id, exisitng, SectorFileType::FTNone);

    if (!locked) {
      return ManagerErrors::kCannotLock;
    }

    OUTCOME_TRY(res,
                local_store_->acquireSector(sector_id,
                                            seal_proof_type_,
                                            exisitng,
                                            allocate,
                                            path,
                                            AcquireMode::kMove));

    return Response{.paths = res.paths, .lock = std::move(locked)};
  }

  std::shared_ptr<proofs::ProofEngine> ManagerImpl::getProofEngine() const {
    return proofs_;
  }

}  // namespace fc::sector_storage

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage, ManagerErrors, e) {
  using fc::sector_storage::ManagerErrors;
  switch (e) {
    case (ManagerErrors::kCannotGetHomeDir):
      return "Manager: cannot get HOME dir to expand path";
    case (ManagerErrors::kSomeSectorSkipped):
      return "Manager: some of sectors was skipped during generating of "
             "winning PoSt";
    case (ManagerErrors::kCannotLock):
      return "Manager: cannot lock sector";
    case (ManagerErrors::kReadOnly):
      return "Manager: read-only storage";
    case (ManagerErrors::kCannotReadData):
      return "Manager: failed to read unsealed piece";
    default:
      return "Manager: unknown error";
  }
}
