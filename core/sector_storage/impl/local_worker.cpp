/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/local_worker.hpp"

namespace fc::sector_storage {

  LocalWorker::LocalWorker(const primitives::WorkerConfig &config,
                           std::shared_ptr<stores::Store> store,
                           std::shared_ptr<stores::LocalStore> local,
                           std::shared_ptr<stores::SectorIndex> sector_index)
      : storage_(std::move(store)),
        local_store_(std::move(local)),
        index_(std::move(sector_index)),
        config_(config) {}

  outcome::result<sector_storage::PreCommit1Output>
  sector_storage::LocalWorker::sealPreCommit1(
      const SectorId &sector,
      const primitives::sector::SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    return outcome::success();
  }

  outcome::result<sector_storage::SectorCids>
  sector_storage::LocalWorker::sealPreCommit2(
      const SectorId &sector, const sector_storage::PreCommit1Output &pc1o) {
    return outcome::success();
  }

  outcome::result<sector_storage::Commit1Output>
  sector_storage::LocalWorker::sealCommit1(
      const SectorId &sector,
      const primitives::sector::SealRandomness &ticket,
      const primitives::sector::InteractiveRandomness &seed,
      gsl::span<const PieceInfo> pieces,
      const sector_storage::SectorCids &cids) {
    return outcome::success();
  }

  outcome::result<primitives::sector::Proof>
  sector_storage::LocalWorker::sealCommit2(
      const SectorId &sector, const sector_storage::Commit1Output &c1o) {
    return outcome::success();
  }

  outcome::result<void> sector_storage::LocalWorker::finalizeSector(
      const SectorId &sector, const gsl::span<Range> &keep_unsealed) {
    return outcome::success();
  }

  outcome::result<void> sector_storage::LocalWorker::releaseUnsealed(
      const SectorId &sector, const gsl::span<Range> &safe_to_free) {
    return outcome::success();
  }

  outcome::result<void> sector_storage::LocalWorker::moveStorage(
      const SectorId &sector) {
    return storage_->moveStorage(
        sector,
        config_.seal_proof_type,
        static_cast<SectorFileType>(SectorFileType::FTCache
                                    | SectorFileType::FTSealed));
  }

  outcome::result<void> sector_storage::LocalWorker::fetch(
      const SectorId &sector,
      const primitives::sector_file::SectorFileType &file_type,
      bool can_seal) {
    return outcome::success();
  }

  outcome::result<void> sector_storage::LocalWorker::unsealPiece(
      const SectorId &sector,
      primitives::piece::UnpaddedByteIndex index,
      const primitives::piece::UnpaddedPieceSize &size,
      const primitives::sector::SealRandomness &randomness,
      const CID &cid) {
    return outcome::success();
  }

  outcome::result<void> sector_storage::LocalWorker::readPiece(
      common::Buffer &output,
      const SectorId &sector,
      primitives::piece::UnpaddedByteIndex index,
      const primitives::piece::UnpaddedPieceSize &size) {
    return outcome::success();
  }

  outcome::result<primitives::WorkerInfo>
  sector_storage::LocalWorker::getInfo() {
    std::string hostname = config_.hostname;
    if (hostname.empty()) {
      // TODO: Get hostname
    }

    OUTCOME_TRY(devices, proofs::Proofs::getGPUDevices());

    return primitives::WorkerInfo{
        .hostname = hostname,
        .resources =
            primitives::WorkerResources{
                .physical_memory = 0,
                .swap_memory = 0,
                .reserved_memory = 0,
                .cpus = 0,
                .gpus = devices,
            },
    };
  }

  outcome::result<std::vector<primitives::TaskType>>
  sector_storage::LocalWorker::getSupportedTask() {
    return config_.task_types;
  }

  outcome::result<std::vector<primitives::StoragePath>>
  sector_storage::LocalWorker::getAccessiblePaths() {
    return outcome::success();
  }

  outcome::result<void> sector_storage::LocalWorker::remove(
      const SectorId &sector) {
    bool isError = false;

    auto cache_err = storage_->remove(sector, SectorFileType::FTCache);
    if (cache_err.has_error()) {
      isError = true;
      // TODO: Log it
    }

    auto sealed_err = storage_->remove(sector, SectorFileType::FTSealed);
    if (sealed_err.has_error()) {
      isError = true;
      // TODO: Log it
    }

    auto unsealed_err = storage_->remove(sector, SectorFileType::FTUnsealed);
    if (unsealed_err.has_error()) {
      isError = true;
      // TODO: Log it
    }

    if (isError) {
      return outcome::success();  // TODO: Error
    }
    return outcome::success();
  }

  outcome::result<void> sector_storage::LocalWorker::newSector(
      const SectorId &sector) {
    return outcome::success();
  }

  outcome::result<PieceInfo> sector_storage::LocalWorker::addPiece(
      const SectorId &sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      const primitives::piece::UnpaddedPieceSize &new_piece_size,
      const primitives::piece::PieceData &piece_data) {
    return outcome::success();
  }
}  // namespace fc::sector_storage
