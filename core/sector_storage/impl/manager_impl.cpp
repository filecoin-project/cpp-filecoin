/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/manager_impl.hpp"

#include <boost/filesystem.hpp>
#include <unordered_set>

namespace fs = boost::filesystem;
using fc::primitives::sector_file::SectorFileType;

namespace fc::sector_storage {

  outcome::result<std::vector<SectorId>> ManagerImpl::checkProvable(
      RegisteredProof seal_proof_type, gsl::span<const SectorId> sectors) {
    return outcome::success();
  }

  SectorSize ManagerImpl::getSectorSize() {
    auto maybe_size = primitives::sector::getSectorSize(seal_proof_type_);
    if (maybe_size.has_value()) {
      return maybe_size.value();
    }
    return 0;
  }

  outcome::result<void> ManagerImpl::ReadPiece(const proofs::PieceData &output,
                                               const SectorId &sector,
                                               UnpaddedByteIndex offset,
                                               const UnpaddedPieceSize &size,
                                               const SealRandomness &randomness,
                                               const CID &cid) {
    return outcome::success();
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
      return outcome::success();  // TODO: ERROR
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

    // TODO: create alloc selector

    // TODO: schedule it
    return outcome::success();
  }

  outcome::result<SectorCids> ManagerImpl::sealPreCommit2(
      const SectorId &sector, const PreCommit1Output &pre_commit_1_output) {
    OUTCOME_TRY(lock,
                index_->storageLock(
                    sector, SectorFileType::FTSealed, SectorFileType::FTCache));

    // TODO: create existing selector

    // TODO: schedule it
    return outcome::success();
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

    // TODO: create existing selector

    // TODO: schedule it
    return outcome::success();
  }

  outcome::result<Proof> ManagerImpl::sealCommit2(
      const SectorId &sector, const Commit1Output &commit_1_output) {
    // TODO: create task selector

    // TODO: schedule it
    return outcome::success();
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

    // TODO: create existing selector

    // TODO: schedule it

    // TODO: create allocate selector

    // TODO: schedule move

    return outcome::success();
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

    // TODO: create existing selector

    // TODO: schedule it

    return outcome::success();
  }

  outcome::result<PieceInfo> ManagerImpl::addPiece(
      const SectorId &sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      const proofs::PieceData &piece_data) {
    return outcome::success();
  }

  outcome::result<void> ManagerImpl::addLocalStorage(const std::string &path) {
    // TODO: expand path

    OUTCOME_TRY(local_store_->openPath(path));

    OUTCOME_TRY(
        local_storage_->setStorage([&path](stores::StorageConfig &config) {
          config.storage_paths.push_back(path);
        }));

    return outcome::success();
  }

  outcome::result<void> ManagerImpl::addWorker(std::shared_ptr<Worker> worker) {
    OUTCOME_TRY(info, worker->getInfo());

    // TODO: add scheduler

    return outcome::success();
  }

  outcome::result<std::unordered_map<StorageID, std::string>>
  ManagerImpl::getStorageLocal() {
    OUTCOME_TRY(paths, local_store_->getAccessiblePaths());

    std::unordered_map<StorageID, std::string> out{};

    for (const auto &path : paths) {
      out[path.id] = path.local_path;
    }

    return std::move(out);
  }

  outcome::result<FsStat> ManagerImpl::getFsStat(StorageID storage_id) {
    return storage_->getFsStat(storage_id);
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
        // TODO: log it
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
}  // namespace fc::sector_storage
