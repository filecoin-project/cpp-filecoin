/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/local_worker.hpp"

#include <boost/filesystem.hpp>
#include "proofs/proofs.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fc::sector_storage {

  LocalWorker::LocalWorker(primitives::WorkerConfig config,
                           std::shared_ptr<stores::Store> store,
                           std::shared_ptr<stores::LocalStore> local,
                           std::shared_ptr<stores::SectorIndex> sector_index)
      : storage_(std::move(store)),
        local_store_(std::move(local)),
        index_(std::move(sector_index)),
        config_(std::move(config)) {}

  outcome::result<sector_storage::PreCommit1Output>
  sector_storage::LocalWorker::sealPreCommit1(
      const SectorId &sector,
      const primitives::sector::SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    OUTCOME_TRY(storage_->remove(sector, SectorFileType::FTSealed));
    OUTCOME_TRY(storage_->remove(sector, SectorFileType::FTCache));

    OUTCOME_TRY(response,
                storage_->acquireSector(
                    sector,
                    config_.seal_proof_type,
                    SectorFileType::FTUnsealed,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    true));

    OUTCOME_TRY(index_->storageDeclareSector(
        response.stores.cache, sector, SectorFileType::FTCache));
    OUTCOME_TRY(index_->storageDeclareSector(
        response.stores.sealed, sector, SectorFileType::FTSealed));

    boost::filesystem::ofstream sealed_file(response.paths.sealed);
    if (!sealed_file.good()) {
      return outcome::success();  // TODO: ERROR
    }
    sealed_file.close();

    if (!boost::filesystem::create_directory(response.paths.cache)) {
      if (boost::filesystem::exists(response.paths.cache)) {
        boost::system::error_code ec;
        boost::filesystem::remove_all(response.paths.cache, ec);
        if (ec.failed()) {
          return outcome::success();  // TODO: ERROR
        }
        if (!boost::filesystem::create_directory(response.paths.cache)) {
          return outcome::success();  // TODO: ERROR
        }
      } else {
        return outcome::success();  // TODO: ERROR
      }
    }

    UnpaddedPieceSize sum;
    for (const auto &piece : pieces) {
      sum += piece.size.unpadded();
    }

    OUTCOME_TRY(size,
                primitives::sector::getSectorSize(config_.seal_proof_type));

    if (sum != primitives::piece::PaddedPieceSize(size).unpadded()) {
      return outcome::success();  // TODO: ERROR
    }

    return proofs::Proofs::sealPreCommitPhase1(config_.seal_proof_type,
                                               response.paths.cache,
                                               response.paths.unsealed,
                                               response.paths.sealed,
                                               sector.sector,
                                               sector.miner,
                                               ticket,
                                               pieces);
  }

  outcome::result<sector_storage::SectorCids>
  sector_storage::LocalWorker::sealPreCommit2(
      const SectorId &sector, const sector_storage::PreCommit1Output &pc1o) {
    OUTCOME_TRY(response,
                storage_->acquireSector(
                    sector,
                    config_.seal_proof_type,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTNone,
                    true));

    return proofs::Proofs::sealPreCommitPhase2(
        pc1o, response.paths.cache, response.paths.sealed);
  }

  outcome::result<sector_storage::Commit1Output>
  sector_storage::LocalWorker::sealCommit1(
      const SectorId &sector,
      const primitives::sector::SealRandomness &ticket,
      const primitives::sector::InteractiveRandomness &seed,
      gsl::span<const PieceInfo> pieces,
      const sector_storage::SectorCids &cids) {
    OUTCOME_TRY(response,
                storage_->acquireSector(
                    sector,
                    config_.seal_proof_type,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTNone,
                    true));

    return proofs::Proofs::sealCommitPhase1(config_.seal_proof_type,
                                            cids.sealed_cid,
                                            cids.unsealed_cid,
                                            response.paths.cache,
                                            response.paths.sealed,
                                            sector.sector,
                                            sector.miner,
                                            ticket,
                                            seed,
                                            pieces);
  }

  outcome::result<primitives::sector::Proof>
  sector_storage::LocalWorker::sealCommit2(
      const SectorId &sector, const sector_storage::Commit1Output &c1o) {
    return proofs::Proofs::sealCommitPhase2(c1o, sector.sector, sector.miner);
  }

  outcome::result<void> sector_storage::LocalWorker::finalizeSector(
      const SectorId &sector, const gsl::span<Range> &keep_unsealed) {
    OUTCOME_TRY(size,
                primitives::sector::getSectorSize(config_.seal_proof_type));

    OUTCOME_TRY(response,
                storage_->acquireSector(sector,
                                        config_.seal_proof_type,
                                        SectorFileType::FTCache,
                                        SectorFileType::FTNone,
                                        false));

    OUTCOME_TRY(proofs::Proofs::clearCache(size, response.paths.cache));

    return storage_->remove(sector, SectorFileType::FTUnsealed);
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
    OUTCOME_TRY(storage_->acquireSector(sector,
                                        config_.seal_proof_type,
                                        file_type,
                                        SectorFileType::FTNone,
                                        can_seal));
    return outcome::success();
  }

  outcome::result<void> sector_storage::LocalWorker::unsealPiece(
      const SectorId &sector,
      primitives::piece::UnpaddedByteIndex offset,
      const primitives::piece::UnpaddedPieceSize &size,
      const primitives::sector::SealRandomness &randomness,
      const CID &cid) {
    auto maybe_unseal_file = storage_->acquireSector(sector,
                                                     config_.seal_proof_type,
                                                     SectorFileType::FTUnsealed,
                                                     SectorFileType::FTNone,
                                                     false);
    if (maybe_unseal_file.has_error()) {
      if (maybe_unseal_file
          != outcome::failure(
              stores::StoreErrors::NotFoundRequestedSectorType)) {
        return maybe_unseal_file.error();
      }
      maybe_unseal_file = storage_->acquireSector(sector,
                                                  config_.seal_proof_type,
                                                  SectorFileType::FTUnsealed,
                                                  SectorFileType::FTNone,
                                                  false);
      if (maybe_unseal_file.has_error()) {
        return maybe_unseal_file.error();
      }
    }

    OUTCOME_TRY(response,
                storage_->acquireSector(
                    sector,
                    config_.seal_proof_type,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTNone,
                    false));

    return proofs::Proofs::unsealRange(config_.seal_proof_type,
                                       response.paths.cache,
                                       response.paths.sealed,
                                       maybe_unseal_file.value().paths.unsealed,
                                       sector.sector,
                                       sector.miner,
                                       randomness,
                                       cid,
                                       primitives::piece::paddedIndex(offset),
                                       size.padded());
  }

  outcome::result<void> sector_storage::LocalWorker::readPiece(
      const proofs::PieceData &output,
      const SectorId &sector,
      primitives::piece::UnpaddedByteIndex offset,
      const primitives::piece::UnpaddedPieceSize &size) {
    if (!output.isOpened()) {
      return outcome::success();  // TODO: ERROR
    }

    OUTCOME_TRY(response,
                storage_->acquireSector(sector,
                                        config_.seal_proof_type,
                                        SectorFileType::FTUnsealed,
                                        SectorFileType::FTNone,
                                        false));

    OUTCOME_TRY(max_size,
                primitives::sector::getSectorSize(config_.seal_proof_type));

    if (primitives::piece::paddedIndex(offset) + size.padded() > max_size) {
      return outcome::success();  // TODO: ERROR
    }

    std::ifstream input(response.paths.unsealed,
                        std::ios_base::in | std::ios_base::binary);

    if (!input.good()) {
      return outcome::success();  // TODO: ERROR
    }

    input.seekg(primitives::piece::paddedIndex(offset), std::ios_base::beg);

    constexpr uint64_t chunk_size = 256;
    char buffer[chunk_size];

    auto sealed_file_fd = output.getFd();

    for (uint64_t read_size = 0; read_size < size;) {
      uint64_t curr_read_size = std::min(chunk_size, size - read_size);

      // read data as a block:
      input.read(buffer, curr_read_size);

      auto read_bytes = input.gcount();

      write(sealed_file_fd, buffer, read_bytes);

      read_size += read_bytes;
    }

    input.close();

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
    return local_store_->getAccessiblePaths();
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
    if (piece_sizes.empty()) {
      OUTCOME_TRY(response,
                  storage_->acquireSector(sector,
                                          config_.seal_proof_type,
                                          SectorFileType::FTNone,
                                          SectorFileType::FTUnsealed,
                                          true));

      OUTCOME_TRY(
          write_response,
          proofs::Proofs::writeWithoutAlignment(config_.seal_proof_type,
                                                piece_data,
                                                new_piece_size,
                                                response.paths.unsealed));

      OUTCOME_TRY(index_->storageDeclareSector(
          response.stores.unsealed, sector, SectorFileType::FTUnsealed));

      return PieceInfo{.size = new_piece_size.padded(),
                       .cid = write_response.piece_cid};
    }

    OUTCOME_TRY(response,
                storage_->acquireSector(sector,
                                        config_.seal_proof_type,
                                        SectorFileType::FTUnsealed,
                                        SectorFileType::FTNone,
                                        true));

    OUTCOME_TRY(write_response,
                proofs::Proofs::writeWithAlignment(config_.seal_proof_type,
                                                   piece_data,
                                                   new_piece_size,
                                                   response.paths.unsealed,
                                                   piece_sizes));

    return PieceInfo{.size = new_piece_size.padded(),
                     .cid = write_response.piece_cid};
  }
}  // namespace fc::sector_storage
