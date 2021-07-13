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

  auto CallError = ERROR_TEXT("Call is failed");

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
    return WorkerAction();
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
      RegisteredPoStProof proof_type, gsl::span<const SectorRef> sectors) {
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

  outcome::result<void> ManagerImpl::remove(const SectorRef &sector) {
    OUTCOME_TRY(index_->storageLock(
        sector.id,
        SectorFileType::FTNone,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTUnsealed
                                    | SectorFileType::FTCache)));

    bool isError = false;

    auto cache_err = remote_store_->remove(sector.id, SectorFileType::FTCache);
    if (cache_err.has_error()) {
      isError = true;
      logger_->error("removing cached sector {} : {}",
                     primitives::sector_file::sectorName(sector.id),
                     cache_err.error().message());
    }

    auto sealed_err =
        remote_store_->remove(sector.id, SectorFileType::FTSealed);
    if (sealed_err.has_error()) {
      isError = true;
      logger_->error("removing sealed sector {} : {}",
                     primitives::sector_file::sectorName(sector.id),
                     sealed_err.error().message());
    }

    auto unsealed_err =
        remote_store_->remove(sector.id, SectorFileType::FTUnsealed);
    if (unsealed_err.has_error()) {
      isError = true;
      logger_->error("removing unsealed sector {} : {}",
                     primitives::sector_file::sectorName(sector.id),
                     unsealed_err.error().message());
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
      gsl::span<const SectorInfo> sector_info,
      const std::function<outcome::result<RegisteredPoStProof>(
          RegisteredSealProof)> &to_post_transform) {
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

      auto res =
          acquireSector(sector_ref,
                        static_cast<SectorFileType>(SectorFileType::FTCache
                                                    | SectorFileType::FTSealed),
                        SectorFileType::FTNone,
                        PathType::kStorage);
      if (res.has_error()) {
        logger_->warn("failed to acquire sector {}", sectorName(sector_ref.id));
        result.skipped.push_back(sector_ref.id);
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
      std::shared_ptr<boost::asio::io_context> io_context,
      const std::shared_ptr<stores::RemoteStore> &remote,
      const std::shared_ptr<Scheduler> &scheduler,
      const SealerConfig &config,
      const std::shared_ptr<proofs::ProofEngine> &proofs) {
    struct make_unique_enabler : public ManagerImpl {
      make_unique_enabler(std::shared_ptr<stores::SectorIndex> sector_index,
                          std::shared_ptr<stores::LocalStorage> local_storage,
                          std::shared_ptr<stores::LocalStore> local_store,
                          std::shared_ptr<stores::RemoteStore> store,
                          std::shared_ptr<Scheduler> scheduler,
                          std::shared_ptr<proofs::ProofEngine> proofs)
          : ManagerImpl{std::move(sector_index),
                        std::move(local_storage),
                        std::move(local_store),
                        std::move(store),
                        std::move(scheduler),
                        std::move(proofs)} {};
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

    std::shared_ptr<Worker> worker =
        LocalWorker::newLocalWorker(io_context,
                                    WorkerConfig{
                                        .custom_hostname = boost::none,
                                        .task_types = std::move(local_tasks),
                                    },
                                    scheduler,
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

  outcome::result<void> ManagerImpl::readPiece(
      PieceData output,
      const SectorRef &sector,
      UnpaddedByteIndex offset,
      const UnpaddedPieceSize &size,
      const SealRandomness &randomness,
      const CID &cid,
      std::function<void(outcome::result<bool>)> cb) {
    OUTCOME_TRY(lock,
                index_->storageLock(
                    sector.id,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTUnsealed));

    {
      OUTCOME_TRY(best,
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

      // TODO: Optimization: don't send unseal to a worker if the requested
      // range is already unsealed

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

      OUTCOME_TRY(scheduler_->schedule(
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
          })));

      auto maybe_error = wait.get_future().get();
      if (maybe_error.has_error()) {
        return maybe_error.error();
      }
    }

    std::shared_ptr<WorkerSelector> selector;
    selector = std::make_shared<ExistingSelector>(
        index_, sector.id, SectorFileType::FTUnsealed, false);

    OUTCOME_TRY(scheduler_->schedule(
        sector,
        primitives::kTTReadUnsealed,
        selector,
        schedFetch(sector,
                   SectorFileType::FTUnsealed,
                   PathType::kSealing,
                   AcquireMode::kMove),
        [&, lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->readPiece(std::move(output), sector, offset, size);
        },
        callbackWrapper(cb)));

    return outcome::success();
  }

  outcome::result<bool> ManagerImpl::readPieceSync(
      PieceData output,
      const SectorRef &sector,
      UnpaddedByteIndex offset,
      const UnpaddedPieceSize &size,
      const SealRandomness &randomness,
      const CID &cid) {
    std::promise<outcome::result<bool>> wait;

    OUTCOME_TRY(
        readPiece(std::move(output),
                  sector,
                  offset,
                  size,
                  randomness,
                  cid,
                  [&wait](outcome::result<bool> res) { wait.set_value(res); }));

    auto res = wait.get_future().get();

    if (res.has_error()) {
      return res.error();
    }

    if (res.value()) {
      return outcome::success();
    }

    return ManagerErrors::kCannotReadData;
  }

  outcome::result<void> ManagerImpl::sealPreCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces,
      std::function<void(outcome::result<PreCommit1Output>)> cb,
      uint64_t priority) {
    OUTCOME_TRY(lock,
                index_->storageLock(
                    sector.id,
                    SectorFileType::FTUnsealed,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache)));

    // TODO: also consider where the unsealed data sits

    auto selector = std::make_unique<AllocateSelector>(
        index_,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        PathType::kSealing);

    return scheduler_->schedule(
        sector,
        primitives::kTTPreCommit1,
        std::move(selector),
        schedFetch(sector,
                   SectorFileType::FTUnsealed,
                   PathType::kSealing,
                   AcquireMode::kMove),
        [&, lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->sealPreCommit1(sector, ticket, pieces);
        },
        callbackWrapper(cb),
        priority);
  }

  outcome::result<PreCommit1Output> ManagerImpl::sealPreCommit1Sync(
      const SectorRef &sector,
      const SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces,
      uint64_t priority) {
    std::promise<outcome::result<PreCommit1Output>> wait;

    OUTCOME_TRY(sealPreCommit1(
        sector,
        ticket,
        pieces,
        [&wait](outcome::result<PreCommit1Output> res) -> void {
          wait.set_value(res);
        },
        priority));

    return wait.get_future().get();
  }

  outcome::result<void> ManagerImpl::sealPreCommit2(
      const SectorRef &sector,
      const PreCommit1Output &pre_commit_1_output,
      std::function<void(outcome::result<SectorCids>)> cb,
      uint64_t priority) {
    OUTCOME_TRY(
        lock,
        index_->storageLock(
            sector.id, SectorFileType::FTSealed, SectorFileType::FTCache));

    auto selector = std::make_shared<ExistingSelector>(
        index_,
        sector.id,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        true);

    return scheduler_->schedule(
        sector,
        primitives::kTTPreCommit2,
        std::move(selector),
        schedFetch(sector,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache),
                   PathType::kSealing,
                   AcquireMode::kMove),
        [&, lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->sealPreCommit2(sector, pre_commit_1_output);
        },
        callbackWrapper(cb),
        priority);
  }

  outcome::result<SectorCids> ManagerImpl::sealPreCommit2Sync(
      const SectorRef &sector,
      const PreCommit1Output &pre_commit_1_output,
      uint64_t priority) {
    std::promise<outcome::result<SectorCids>> wait;

    OUTCOME_TRY(sealPreCommit2(
        sector,
        pre_commit_1_output,
        [&wait](outcome::result<SectorCids> res) -> void {
          wait.set_value(res);
        },
        priority));

    return wait.get_future().get();
  }

  outcome::result<void> ManagerImpl::sealCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      const InteractiveRandomness &seed,
      gsl::span<const PieceInfo> pieces,
      const SectorCids &cids,
      std::function<void(outcome::result<Commit1Output>)> cb,
      uint64_t priority) {
    OUTCOME_TRY(
        lock,
        index_->storageLock(
            sector.id, SectorFileType::FTSealed, SectorFileType::FTCache));

    auto selector = std::make_shared<ExistingSelector>(
        index_,
        sector.id,
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
        [&, lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->sealCommit1(sector, ticket, seed, pieces, cids);
        },
        callbackWrapper(cb),
        priority));

    return outcome::success();
  }

  outcome::result<Commit1Output> ManagerImpl::sealCommit1Sync(
      const SectorRef &sector,
      const SealRandomness &ticket,
      const InteractiveRandomness &seed,
      gsl::span<const PieceInfo> pieces,
      const SectorCids &cids,
      uint64_t priority) {
    std::promise<outcome::result<Commit1Output>> wait;
    OUTCOME_TRY(sealCommit1(
        sector,
        ticket,
        seed,
        pieces,
        cids,
        [&wait](outcome::result<Commit1Output> res) -> void {
          wait.set_value(res);
        },
        priority));
    return wait.get_future().get();
  }

  outcome::result<void> ManagerImpl::sealCommit2(
      const SectorRef &sector,
      const Commit1Output &commit_1_output,
      std::function<void(outcome::result<Proof>)> cb,
      uint64_t priority) {
    std::unique_ptr<TaskSelector> selector = std::make_unique<TaskSelector>();

    return scheduler_->schedule(
        sector,
        primitives::kTTCommit2,
        std::move(selector),
        schedNothing(),
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->sealCommit2(sector, commit_1_output);
        },
        callbackWrapper(cb),
        priority);
  }

  outcome::result<Proof> ManagerImpl::sealCommit2Sync(
      const SectorRef &sector,
      const Commit1Output &commit_1_output,
      uint64_t priority) {
    std::promise<outcome::result<Proof>> wait;

    OUTCOME_TRY(sealCommit2(
        sector,
        commit_1_output,
        [&wait](outcome::result<Proof> res) -> void { wait.set_value(res); },
        priority));

    return wait.get_future().get();
  }

  outcome::result<void> ManagerImpl::finalizeSector(
      const SectorRef &sector,
      const gsl::span<const Range> &keep_unsealed,
      std::function<void(outcome::result<void>)> cb,
      uint64_t priority) {
    OUTCOME_TRY(lock,
                index_->storageLock(
                    sector.id,
                    SectorFileType::FTNone,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTUnsealed
                                                | SectorFileType::FTCache)));

    auto unsealed = SectorFileType::FTUnsealed;
    {
      OUTCOME_TRY(unsealed_stores,
                  index_->storageFindSector(
                      sector.id, SectorFileType::FTUnsealed, boost::none));

      if (unsealed_stores.empty()) {
        unsealed = SectorFileType::FTNone;
      }
    }

    std::shared_ptr<WorkerSelector> selector;

    selector = std::make_shared<ExistingSelector>(
        index_,
        sector.id,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        false);

    std::promise<outcome::result<void>> waiter;

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
        [&](const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->finalizeSector(sector, keep_unsealed);
        },
        callbackWrapper([&waiter](outcome::result<void> maybe_err) -> void {
          waiter.set_value(maybe_err);
        }),
        priority));

    auto maybe_error = waiter.get_future().get();
    if (maybe_error.has_error()) return maybe_error.error();

    auto fetch_selector = std::make_shared<AllocateSelector>(
        index_,
        static_cast<SectorFileType>(SectorFileType::FTSealed
                                    | SectorFileType::FTCache),
        PathType::kStorage);

    auto moveUnsealed = unsealed;
    if (keep_unsealed.empty()) {
      moveUnsealed = SectorFileType::FTNone;
    }

    return scheduler_->schedule(
        sector,
        primitives::kTTFetch,
        fetch_selector,
        schedFetch(sector,
                   static_cast<SectorFileType>(SectorFileType::FTSealed
                                               | SectorFileType::FTCache),
                   PathType::kStorage,
                   AcquireMode::kMove),
        [&, lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->moveStorage(
              sector,
              static_cast<SectorFileType>(SectorFileType::FTSealed
                                          | SectorFileType::FTCache
                                          | moveUnsealed));
        },
        callbackWrapper(cb),
        priority);
  }

  outcome::result<void> ManagerImpl::finalizeSectorSync(
      const SectorRef &sector,
      const gsl::span<const Range> &keep_unsealed,
      uint64_t priority) {
    std::promise<outcome::result<void>> waiter;

    OUTCOME_TRY(finalizeSector(
        sector,
        keep_unsealed,
        [&waiter](outcome::result<void> res) -> void { waiter.set_value(res); },
        priority));

    return waiter.get_future().get();
  }

  outcome::result<void> ManagerImpl::addPiece(
      const SectorRef &sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      proofs::PieceData piece_data,
      std::function<void(outcome::result<PieceInfo>)> cb,
      uint64_t priority) {
    OUTCOME_TRY(
        lock,
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

    return scheduler_->schedule(
        sector,
        primitives::kTTAddPiece,
        selector,
        schedNothing(),
        [sector,
         piece_sizes,
         new_piece_size,
         data = std::make_shared<PieceData>(std::move(piece_data)),
         lock = std::move(lock)](
            const std::shared_ptr<Worker> &worker) -> outcome::result<CallId> {
          return worker->addPiece(
              sector, piece_sizes, new_piece_size, std::move(*data));
        },
        callbackWrapper(cb),
        priority);
  }

  outcome::result<PieceInfo> ManagerImpl::addPieceSync(
      const SectorRef &sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      proofs::PieceData piece_data,
      uint64_t priority) {
    std::promise<outcome::result<PieceInfo>> wait_result;

    OUTCOME_TRY(addPiece(
        sector,
        piece_sizes,
        new_piece_size,
        std::move(piece_data),
        [&wait_result](outcome::result<PieceInfo> maybe_pi) -> void {
          wait_result.set_value(maybe_pi);
        },
        priority));

    return wait_result.get_future().get();
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
