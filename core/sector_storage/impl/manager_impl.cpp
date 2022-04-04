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
#include <string>
#include <unordered_set>

#include "api/storage_miner/return_api.hpp"
#include "codec/json/json.hpp"
#include "common/outcome_fmt.hpp"
#include "common/put_in_function.hpp"
#include "sector_storage/impl/allocate_selector.hpp"
#include "sector_storage/impl/existing_selector.hpp"
#include "sector_storage/impl/local_worker.hpp"
#include "sector_storage/impl/task_selector.hpp"
#include "sector_storage/schedulder_utils.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fc::sector_storage {
  using common::PutInFunction;
  using primitives::sector::SectorInfo;
  using primitives::sector::toSectorInfo;
  using primitives::sector_file::SectorFileType;
  using primitives::sector_file::sectorName;
  namespace fs = boost::filesystem;

  WorkerAction schedFetch(const SectorRef &sector,
                          SectorFileType file_type,
                          PathType path_type,
                          AcquireMode mode) {
    return [=](const std::shared_ptr<fc::sector_storage::Worker> &worker)
               -> outcome::result<CallId> {
      return worker->fetch(sector, file_type, path_type, mode);
    };
  }

  WorkerAction schedNothing() {
    return {};
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
      if (pwd != nullptr) {
        home_dir = pwd->pw_dir;
      } else {
        return fc::sector_storage::ManagerErrors::kCannotGetHomeDir;
      }
    } else {
      home_dir = home;
    }

    return (fs::path(home_dir) / path.substr(1, path.size() - 1)).string();
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<std::vector<SectorId>> ManagerImpl::checkProvable(
      RegisteredPoStProof proof_type,
      gsl::span<const SectorRef> sectors) const {
    std::vector<SectorId> bad{};

    OUTCOME_TRY(ssize, primitives::sector::getSectorSize(proof_type));

    for (const auto &sector : sectors) {
      auto locked = index_->storageTryLock(
          sector.id,
          static_cast<SectorFileType>(SectorFileType::FTSealed
                                      | SectorFileType::FTCache),
          SectorFileType::FTNone);

      if (!locked) {
        logger_->warn("can't acquire read lock for {} sector",
                      sectorName(sector.id));
        bad.push_back(sector.id);
        continue;
      }

      auto maybe_response = local_store_->acquireSector(
          sector,
          static_cast<SectorFileType>(SectorFileType::FTSealed
                                      | SectorFileType::FTCache),
          SectorFileType::FTNone,
          PathType::kStorage,
          AcquireMode::kMove);

      if (maybe_response.has_error()) {
        if (maybe_response
            == outcome::failure(stores::StoreError::kNotFoundSector)) {
          logger_->warn("cache an/or sealed paths not found for {} sector",
                        sectorName(sector.id));
          bad.push_back(sector.id);
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
              "{} doesnt exist for {} sector", path, sectorName(sector.id));
          bad.push_back(sector.id);
          break;
        }

        if (size != 0) {
          boost::system::error_code ec;
          size_t actual_size = fs::file_size(path, ec);
          if (ec.failed()) {
            logger_->warn("sector {}. Can't get size for {}: {}",
                          sectorName(sector.id),
                          path,
                          ec.message());
            bad.push_back(sector.id);
            break;
          }

          if (actual_size != ssize * size) {
            logger_->warn(
                "sector {}. Actual and declared sizes do not match for {}",
                sectorName(sector.id),
                path);
            bad.push_back(sector.id);
            break;
          }
        }
      }
    }

    return std::move(bad);
  }

  outcome::result<std::vector<PoStProof>> ManagerImpl::generateWinningPoSt(
      ActorId miner_id,
      gsl::span<const ExtendedSectorInfo> sector_info,
      PoStRandomness randomness) {
    OUTCOME_TRY(res, publicSectorToPrivate(miner_id, sector_info, true));

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
      gsl::span<const ExtendedSectorInfo> sector_info,
      PoStRandomness randomness) {
    Prover::WindowPoStResponse response{};

    OUTCOME_TRY(res, publicSectorToPrivate(miner_id, sector_info, false));

    OUTCOME_TRYA(
        response.proof,
        proofs_->generateWindowPoSt(miner_id, res.private_info, randomness));

    response.skipped = std::move(res.skipped);

    return std::move(response);
  }

  outcome::result<void> ManagerImpl::remove(const SectorRef &sector) {
    OUTCOME_TRY(index_->storageLock(
        sector.id,
        SectorFileType::FTNone,
        SectorFileType::FTCache | SectorFileType::FTSealed
            | SectorFileType::FTUnsealed | SectorFileType::FTUpdate
            | SectorFileType::FTUpdateCache));

    bool isError = false;

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      const auto r{remote_store_->remove(sector.id, type)};
      if (r) {
        continue;
      }
      isError = true;
      logger_->error("removing sector {}/{}: {:#}",
                     toString(type),
                     primitives::sector_file::sectorName(sector.id),
                     r.error());
    }

    if (isError) {
      return WorkerErrors::kCannotRemoveSector;
    }
    return outcome::success();
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
      gsl::span<const ExtendedSectorInfo> sector_info,
      bool winning) {
    PubToPrivateResponse result;

    std::vector<proofs::PrivateSectorInfo> out{};
    for (const auto &sector : sector_info) {
      SectorRef sector_ref{
          .id =
              SectorId{
                  .miner = miner,
                  .sector = sector.sector,
              },
          .proof_type = sector.registered_proof,
      };

      SectorFileType sector_file_type = SectorFileType::FTNone;
      if (sector.sector_key.has_value()) {
        logger_->debug("Posting over updated sector for sector id: {}",
                       sector.sector);
        sector_file_type = static_cast<SectorFileType>(
            SectorFileType::FTUpdateCache | SectorFileType::FTUpdate);
      } else {
        logger_->debug("Posting over sector key sector for sector id: {}",
                       sector.sector);
        sector_file_type = static_cast<SectorFileType>(
            SectorFileType::FTCache | SectorFileType::FTSealed);
      }
      const auto res = acquireSector(sector_ref,
                                     sector_file_type,
                                     SectorFileType::FTNone,
                                     PathType::kStorage);
      if (res.has_error()) {
        logger_->warn("failed to acquire sector {}", sectorName(sector_ref.id));
        result.skipped.push_back(sector_ref.id);
        continue;
      }

      OUTCOME_TRY(post_proof_type,
                  winning
                      ? getRegisteredWinningPoStProof(sector.registered_proof)
                      : getRegisteredWindowPoStProof(sector.registered_proof));

      result.locks.push_back(std::move(res.value().lock));

      out.push_back(proofs::PrivateSectorInfo{
          .info = toSectorInfo(sector),
          .cache_dir_path = res.value().paths.cache,
          .post_proof_type = post_proof_type,
          .sealed_sector_path = res.value().paths.sealed,
      });
    }

    result.private_info = proofs::newSortedPrivateSectorInfo(out);

    return std::move(result);
  }

  outcome::result<std::shared_ptr<Manager>> ManagerImpl::newManager(
      const std::shared_ptr<boost::asio::io_context> &io_context,
      const std::shared_ptr<stores::RemoteStore> &remote,
      const std::shared_ptr<Scheduler> &scheduler,
      const SealerConfig &config,
      const std::shared_ptr<proofs::ProofEngine> &proofs) {
    struct make_unique_enabler : public ManagerImpl {
      make_unique_enabler(
          const std::shared_ptr<stores::SectorIndex> &sector_index,
          const std::shared_ptr<stores::LocalStorage> &local_storage,
          const std::shared_ptr<stores::LocalStore> &local_store,
          const std::shared_ptr<stores::RemoteStore> &store,
          const std::shared_ptr<Scheduler> &scheduler,
          const std::shared_ptr<proofs::ProofEngine> &proofs)
          : ManagerImpl{sector_index,
                        local_storage,
                        local_store,
                        store,
                        scheduler,
                        proofs} {};
    };

    auto local_store = remote->getLocalStore();
    auto local_storage = local_store->getLocalStorage();
    auto sector_index = remote->getSectorIndex();
    std::unique_ptr<ManagerImpl> manager =
        std::make_unique<make_unique_enabler>(sector_index,
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

    auto return_api{std::make_shared<WorkerReturn>()};
    api::makeReturnApi(return_api, scheduler);
    std::shared_ptr<Worker> worker =
        std::make_shared<LocalWorker>(io_context,
                                      WorkerConfig{
                                          .custom_hostname = boost::none,
                                          .task_types = std::move(local_tasks),
                                      },
                                      return_api,
                                      remote,
                                      proofs);

    OUTCOME_TRY(manager->addWorker(std::move(worker)));
    return std::move(manager);
  }

  ManagerImpl::ManagerImpl(std::shared_ptr<stores::SectorIndex> sector_index,
                           std::shared_ptr<stores::LocalStorage> local_storage,
                           std::shared_ptr<stores::LocalStore> local_store,
                           std::shared_ptr<stores::RemoteStore> store,
                           std::shared_ptr<Scheduler> scheduler,
                           std::shared_ptr<proofs::ProofEngine> proofs)
      : index_(std::move(sector_index)),
        local_storage_(std::move(local_storage)),
        local_store_(std::move(local_store)),
        remote_store_(std::move(store)),
        scheduler_(std::move(scheduler)),
        logger_(common::createLogger("manager")),
        proofs_(std::move(proofs)) {}

  outcome::result<ManagerImpl::Response> ManagerImpl::acquireSector(
      SectorRef sector,
      SectorFileType exisitng,
      SectorFileType allocate,
      PathType path) {
    if (allocate != SectorFileType::FTNone) {
      return ManagerErrors::kReadOnly;
    }

    auto locked =
        index_->storageTryLock(sector.id, exisitng, SectorFileType::FTNone);

    if (!locked) {
      return ManagerErrors::kCannotLock;
    }

    OUTCOME_TRY(res,
                local_store_->acquireSector(
                    sector, exisitng, allocate, path, AcquireMode::kMove));

    return Response{.paths = res.paths, .lock = std::move(locked)};
  }

  std::shared_ptr<proofs::ProofEngine> ManagerImpl::getProofEngine() const {
    return proofs_;
  }

  void ManagerImpl::readPiece(
      PieceData output,
      const SectorRef &sector,
      UnpaddedByteIndex offset,
      const UnpaddedPieceSize &size,
      const SealRandomness &randomness,
      const CID &cid,
      const std::function<void(outcome::result<bool>)> &cb,
      uint64_t priority) {
    OUTCOME_CB(auto lock,
               index_->storageLock(
                   sector.id,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache),
                   SectorFileType::FTUnsealed));

    {
      OUTCOME_CB(auto best,
                 index_->storageFindSector(
                     sector.id, SectorFileType::FTUnsealed, boost::none));

      std::shared_ptr<WorkerSelector> selector;
      if (best.empty()) {
        selector = std::make_unique<AllocateSelector>(
            index_, SectorFileType::FTUnsealed, PathType::kSealing);
      } else {
        selector = std::make_shared<ExistingSelector>(
            index_, sector.id, SectorFileType::FTUnsealed, false);
      }

      // TODO(ortyomka): Optimization: don't send unseal to a worker if the
      // requested range is already unsealed

      WorkerAction unseal_fetch = [&](const std::shared_ptr<Worker> &worker)
          -> outcome::result<CallId> {
        SectorFileType unsealed =
            best.empty() ? SectorFileType::FTNone : SectorFileType::FTUnsealed;

        return worker->fetch(
            sector,
            static_cast<SectorFileType>(SectorFileType::FTSealed
                                        | SectorFileType::FTCache | unsealed),
            PathType::kSealing,
            AcquireMode::kMove);
      };

      std::promise<outcome::result<void>> wait;

      OUTCOME_CB1(scheduler_->schedule(
          sector,
          primitives::kTTUnseal,
          selector,
          unseal_fetch,
          [sector, offset, size, randomness, cid](
              const std::shared_ptr<Worker> &worker)
              -> outcome::result<CallId> {
            return worker->unsealPiece(sector, offset, size, randomness, cid);
          },
          callbackWrapper([&wait](outcome::result<void> res) -> void {
            wait.set_value(res);
          }),
          priority,
          boost::none));

      OUTCOME_CB1(wait.get_future().get());
    }

    std::shared_ptr<WorkerSelector> selector;
    selector = std::make_shared<ExistingSelector>(
        index_, sector.id, SectorFileType::FTUnsealed, false);

    OUTCOME_CB1(scheduler_->schedule(
        sector,
        primitives::kTTReadUnsealed,
        selector,
        schedFetch(sector,
                   SectorFileType::FTUnsealed,
                   PathType::kSealing,
                   AcquireMode::kMove),
        [sector,
         offset,
         size,
         output = std::make_shared<PieceData>(std::move(output)),
         lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->readPiece(std::move(*output), sector, offset, size);
        },
        callbackWrapper(cb),
        priority,
        boost::none));
  }

  void ManagerImpl::sealPreCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      const std::vector<PieceInfo> &pieces,
      const std::function<void(outcome::result<PreCommit1Output>)> &cb,
      uint64_t priority) {
    OUTCOME_CB(WorkId work_id,
               getWorkId(primitives::kTTPreCommit1,
                         std::make_tuple(sector, ticket, pieces)));

    OUTCOME_CB(auto lock,
               index_->storageLock(
                   sector.id,
                   SectorFileType::FTUnsealed,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache)));

    // TODO(ortyomka): also consider where the unsealed data sits

    auto selector = std::make_unique<AllocateSelector>(
        index_,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        PathType::kSealing);

    OUTCOME_CB1(scheduler_->schedule(
        sector,
        primitives::kTTPreCommit1,
        std::move(selector),
        schedFetch(sector,
                   SectorFileType::FTUnsealed,
                   PathType::kSealing,
                   AcquireMode::kMove),
        [sector, ticket, pieces, lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->sealPreCommit1(sector, ticket, pieces);
        },
        callbackWrapper(cb),
        priority,
        work_id));
  }

  void ManagerImpl::sealPreCommit2(
      const SectorRef &sector,
      const PreCommit1Output &pre_commit_1_output,
      const std::function<void(outcome::result<SectorCids>)> &cb,
      uint64_t priority) {
    OUTCOME_CB(WorkId work_id,
               getWorkId(primitives::kTTPreCommit2,
                         std::make_tuple(sector, pre_commit_1_output)));

    OUTCOME_CB(
        auto lock,
        index_->storageLock(
            sector.id, SectorFileType::FTSealed, SectorFileType::FTCache));

    auto selector = std::make_shared<ExistingSelector>(
        index_,
        sector.id,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        true);

    OUTCOME_CB1(scheduler_->schedule(
        sector,
        primitives::kTTPreCommit2,
        std::move(selector),
        schedFetch(sector,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache),
                   PathType::kSealing,
                   AcquireMode::kMove),
        [sector, pre_commit_1_output, lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->sealPreCommit2(sector, pre_commit_1_output);
        },
        callbackWrapper(cb),
        priority,
        work_id))
  }

  void ManagerImpl::sealCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      const InteractiveRandomness &seed,
      const std::vector<PieceInfo> &pieces,
      const SectorCids &cids,
      const std::function<void(outcome::result<Commit1Output>)> &cb,
      uint64_t priority) {
    OUTCOME_CB(WorkId work_id,
               getWorkId(primitives::kTTCommit1,
                         std::make_tuple(sector, ticket, seed, pieces, cids)));

    OUTCOME_CB(
        auto lock,
        index_->storageLock(
            sector.id, SectorFileType::FTSealed, SectorFileType::FTCache));

    auto selector = std::make_shared<ExistingSelector>(
        index_,
        sector.id,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        false);

    OUTCOME_CB1(scheduler_->schedule(
        sector,
        primitives::kTTCommit1,
        std::move(selector),
        schedFetch(sector,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache),
                   PathType::kSealing,
                   AcquireMode::kMove),
        [sector, ticket, seed, pieces, cids, lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->sealCommit1(sector, ticket, seed, pieces, cids);
        },
        callbackWrapper(cb),
        priority,
        work_id));
  }

  void ManagerImpl::sealCommit2(
      const SectorRef &sector,
      const Commit1Output &commit_1_output,
      const std::function<void(outcome::result<Proof>)> &cb,
      uint64_t priority) {
    OUTCOME_CB(WorkId work_id,
               getWorkId(primitives::kTTCommit2,
                         std::make_tuple(sector, commit_1_output)));

    auto selector = std::make_unique<TaskSelector>();

    OUTCOME_CB1(scheduler_->schedule(
        sector,
        primitives::kTTCommit2,
        std::move(selector),
        schedNothing(),
        [sector, commit_1_output](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->sealCommit2(sector, commit_1_output);
        },
        callbackWrapper(cb),
        priority,
        work_id));
  }

  void ManagerImpl::replicaUpdate(
      const SectorRef &sector,
      const std::vector<PieceInfo> &pieces,
      const std::function<void(outcome::result<ReplicaUpdateOut>)> &cb,
      uint64_t priority) {
    logger_->debug("sector_storage::Manager is doing replica update");
    OUTCOME_CB(WorkId work_id,
               getWorkId(primitives::kTTReplicaUpdate,
                         std::make_tuple(sector, pieces)));

    OUTCOME_CB(auto lock,
               index_->storageLock(
                   sector.id,
                   SectorFileType::FTUnsealed | SectorFileType::FTSealed
                       | SectorFileType::FTCache,
                   SectorFileType::FTUpdate | SectorFileType::FTUpdateCache));

    auto selector = std::make_unique<AllocateSelector>(
        index_,
        SectorFileType::FTUpdate | SectorFileType::FTUpdateCache,
        PathType::kSealing);

    OUTCOME_CB1(scheduler_->schedule(
        sector,
        primitives::kTTReplicaUpdate,
        std::move(selector),
        schedFetch(sector,
                   SectorFileType::FTUnsealed | SectorFileType::FTSealed
                       | SectorFileType::FTCache,
                   PathType::kSealing,
                   AcquireMode::kCopy),
        [sector, pieces, lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->replicaUpdate(sector, pieces);
        },
        callbackWrapper(cb),
        priority,
        work_id));
  }

  void ManagerImpl::finalizeSectorInner(
      const SectorRef &sector,
      std::vector<Range> keep_unsealed,
      SectorFileType main_type,
      SectorFileType additional_types,
      const std::function<void(outcome::result<void>)> &cb,
      uint64_t priority) {
    OUTCOME_CB(auto lock,
               index_->storageLock(
                   sector.id,
                   SectorFileType::FTNone,
                   static_cast<SectorFileType>(main_type | additional_types
                                               | SectorFileType::FTUnsealed)));

    auto unsealed = SectorFileType::FTUnsealed;
    {
      OUTCOME_CB(auto unsealed_stores,
                 index_->storageFindSector(
                     sector.id, SectorFileType::FTUnsealed, boost::none));

      if (unsealed_stores.empty()) {
        unsealed = SectorFileType::FTNone;
      }
    }

    auto path_type = PathType::kStorage;
    {
      OUTCOME_CB(auto sealed_stores,
                 index_->storageFindSector(
                     sector.id, SectorFileType::FTSealed, boost::none));

      for (const auto &store : sealed_stores) {
        if (store.can_seal) {
          path_type = PathType::kSealing;
          break;
        }
      }
    }

    auto next_cb = [cb,
                    index{index_},
                    unsealed,
                    need_unsealed{not keep_unsealed.empty()},
                    scheduler{scheduler_},
                    sector,
                    lock{std::move(lock)},
                    self{shared_from_this()},
                    priority,
                    main_type,
                    additional_types](
                       const outcome::result<void> &maybe_error) mutable {
      OUTCOME_CB1(maybe_error);

      auto fetch_selector = std::make_shared<AllocateSelector>(
          index,
          static_cast<SectorFileType>(main_type | additional_types),
          PathType::kStorage);

      const auto moveUnsealed =
          need_unsealed ? unsealed : SectorFileType::FTNone;

      OUTCOME_CB1(scheduler->schedule(
          sector,
          primitives::kTTFetch,
          fetch_selector,
          schedFetch(sector,
                     static_cast<SectorFileType>(main_type | additional_types
                                                 | moveUnsealed),
                     PathType::kStorage,
                     AcquireMode::kMove),
          [sector,
           moveUnsealed,
           main_type,
           additional_types,
           lock{std::move(lock)}](const std::shared_ptr<Worker> &worker)
              -> outcome::result<CallId> {
            return worker->moveStorage(
                sector,
                static_cast<SectorFileType>(main_type | additional_types
                                            | moveUnsealed));
          },
          self->callbackWrapper(cb),
          priority,
          boost::none));
    };

    std::shared_ptr<WorkerSelector> selector;

    selector = std::make_shared<ExistingSelector>(
        index_,
        sector.id,
        static_cast<SectorFileType>(main_type | additional_types),
        false);

    OUTCOME_CB1(scheduler_->schedule(
        sector,
        primitives::kTTFinalize,
        selector,
        schedFetch(sector,
                   static_cast<SectorFileType>(main_type | additional_types
                                               | unsealed),
                   path_type,
                   AcquireMode::kMove),
        [sector, keep_unsealed{std::move(keep_unsealed)}](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->finalizeSector(sector, std::move(keep_unsealed));
        },
        callbackWrapper(std::move(next_cb)),
        priority,
        boost::none));
  }

  void ManagerImpl::finalizeSector(
      const SectorRef &sector,
      std::vector<Range> keep_unsealed,
      const std::function<void(outcome::result<void>)> &cb,
      uint64_t priority) {
    finalizeSectorInner(sector,
                        keep_unsealed,
                        SectorFileType::FTSealed,
                        SectorFileType::FTCache,
                        cb,
                        priority);
  }

  void ManagerImpl::proveReplicaUpdate1(
      const SectorRef &sector,
      const CID &sector_key,
      const CID &new_sealed,
      const CID &new_unsealed,
      const std::function<void(outcome::result<ReplicaVanillaProofs>)> &cb,
      uint64_t priority) {
    OUTCOME_CB(WorkId work_id,
               getWorkId(primitives::kTTProveReplicaUpdate1,
                         std::make_tuple(
                             sector, sector_key, new_sealed, new_unsealed)));

    OUTCOME_CB(
        auto lock,
        index_->storageLock(sector.id,
                            SectorFileType::FTSealed | SectorFileType::FTUpdate
                                | SectorFileType::FTCache
                                | SectorFileType::FTUpdateCache,
                            SectorFileType::FTNone));

    // NOTE: We set allowFetch to false in so that we always execute on a worker
    // with direct access to the data. We want to do that because this step is
    // generally very cheap / fast, and transferring data is not worth the
    // effort
    auto selector = std::make_unique<ExistingSelector>(
        index_,
        sector.id,
        SectorFileType::FTUpdate | SectorFileType::FTUpdateCache
            | SectorFileType::FTSealed | SectorFileType::FTCache,
        false);

    OUTCOME_CB1(scheduler_->schedule(
        sector,
        primitives::kTTProveReplicaUpdate1,
        std::move(selector),
        schedFetch(sector,
                   SectorFileType::FTSealed | SectorFileType::FTCache
                       | SectorFileType::FTUpdate
                       | SectorFileType::FTUpdateCache,
                   PathType::kSealing,
                   AcquireMode::kCopy),
        [sector, sector_key, new_sealed, new_unsealed, lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->proveReplicaUpdate1(
              sector, sector_key, new_sealed, new_unsealed);
        },
        callbackWrapper(cb),
        priority,
        work_id));
  }

  void ManagerImpl::proveReplicaUpdate2(
      const SectorRef &sector,
      const CID &sector_key,
      const CID &new_sealed,
      const CID &new_unsealed,
      const Update1Output &update_1_output,
      const std::function<void(outcome::result<ReplicaUpdateProof>)> &cb,
      uint64_t priority) {
    OUTCOME_CB(WorkId work_id,
               getWorkId(primitives::kTTProveReplicaUpdate2,
                         std::make_tuple(sector,
                                         sector_key,
                                         new_sealed,
                                         new_unsealed,
                                         update_1_output)));

    auto selector = std::make_unique<TaskSelector>();

    OUTCOME_CB1(scheduler_->schedule(
        sector,
        primitives::kTTProveReplicaUpdate2,
        std::move(selector),
        schedNothing(),
        [sector, sector_key, new_sealed, new_unsealed, update_1_output](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->proveReplicaUpdate2(
              sector, sector_key, new_sealed, new_unsealed, update_1_output);
        },
        callbackWrapper(cb),
        priority,
        work_id));
  }

  void ManagerImpl::addPiece(
      const SectorRef &sector,
      VectorCoW<UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      proofs::PieceData piece_data,
      const std::function<void(outcome::result<PieceInfo>)> &cb,
      uint64_t priority) {
    OUTCOME_CB(
        auto lock,
        index_->storageLock(
            sector.id, SectorFileType::FTNone, SectorFileType::FTUnsealed));

    std::shared_ptr<WorkerSelector> selector;
    if (piece_sizes.empty()) {
      selector = std::make_unique<AllocateSelector>(
          index_, SectorFileType::FTUnsealed, PathType::kSealing);
    } else {
      selector = std::make_shared<ExistingSelector>(
          index_, sector.id, SectorFileType::FTUnsealed, false);
    }

    OUTCOME_CB1(scheduler_->schedule(
        sector,
        primitives::kTTAddPiece,
        selector,
        schedNothing(),
        PutInFunction([sector,
                       exist_sizes = std::move(piece_sizes),
                       new_piece_size,
                       data = std::move(piece_data),
                       lock = std::move(lock)](
                          const std::shared_ptr<Worker> &worker) mutable
                      -> outcome::result<CallId> {
          return worker->addPiece(
              sector, std::move(exist_sizes), new_piece_size, std::move(data));
        }),
        callbackWrapper(cb),
        priority,
        boost::none));
  }

  outcome::result<PieceInfo> ManagerImpl::addPieceSync(
      const SectorRef &sector,
      VectorCoW<UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      proofs::PieceData piece_data,
      uint64_t priority) {
    std::promise<outcome::result<PieceInfo>> wait_result;

    addPiece(
        sector,
        std::move(piece_sizes),
        new_piece_size,
        std::move(piece_data),
        [&wait_result](const outcome::result<PieceInfo> &maybe_pi) -> void {
          wait_result.set_value(maybe_pi);
        },
        priority);

    return wait_result.get_future().get();
  }

  void ManagerImpl::finalizeReplicaUpdate(
      const SectorRef &sector,
      std::vector<Range> keep_unsealed,
      const std::function<void(outcome::result<void>)> &cb,
      uint64_t priority) {
    finalizeSectorInner(sector,
                        keep_unsealed,
                        SectorFileType::FTUpdate,
                        static_cast<SectorFileType>(
                            SectorFileType::FTSealed | SectorFileType::FTCache
                            | SectorFileType::FTUpdateCache),
                        cb,
                        priority);
  }

  outcome::result<void> ManagerImpl::releaseReplicaUpgrade(
      const SectorRef &sector) {
    OUTCOME_TRY(lock,
                index_->storageLock(
                    sector.id,
                    SectorFileType::FTNone,
                    SectorFileType::FTUpdateCache | SectorFileType::FTUpdate));

    OUTCOME_TRY(
        remote_store_->remove(sector.id, SectorFileType::FTUpdateCache));

    return remote_store_->remove(sector.id, SectorFileType::FTUpdate);
  }

  outcome::result<void> ManagerImpl::releaseSectorKey(const SectorRef &sector) {
    OUTCOME_TRY(
        lock,
        index_->storageLock(
            sector.id, SectorFileType::FTNone, SectorFileType::FTSealed));

    return remote_store_->remove(sector.id, SectorFileType::FTSealed);
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
