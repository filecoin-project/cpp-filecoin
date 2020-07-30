/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/local_worker.hpp"

#if __APPLE__
#include <mach/mach_host.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#endif

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <thread>
#include "proofs/proofs.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fc::sector_storage {

  using primitives::piece::PaddedPieceSize;
  using proofs::PieceData;

  LocalWorker::LocalWorker(WorkerConfig config,
                           std::shared_ptr<stores::RemoteStore> store)
      : remote_store_(std::move(store)),
        local_store_(remote_store_->getLocalStore()),
        index_(local_store_->getSectorIndex()),
        config_(std::move(config)),
        logger_(common::createLogger("local worker")) {}

  outcome::result<sector_storage::PreCommit1Output>
  sector_storage::LocalWorker::sealPreCommit1(
      const SectorId &sector,
      const primitives::sector::SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    OUTCOME_TRY(remote_store_->remove(sector, SectorFileType::FTSealed));
    OUTCOME_TRY(remote_store_->remove(sector, SectorFileType::FTCache));

    OUTCOME_TRY(response,
                remote_store_->acquireSector(
                    sector,
                    config_.seal_proof_type,
                    SectorFileType::FTUnsealed,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    true));

    OUTCOME_TRY(index_->storageDeclareSector(
        response.storages.cache, sector, SectorFileType::FTCache));
    OUTCOME_TRY(index_->storageDeclareSector(
        response.storages.sealed, sector, SectorFileType::FTSealed));

    boost::filesystem::ofstream sealed_file(response.paths.sealed);
    if (!sealed_file.good()) {
      return WorkerErrors::kCannotCreateSealedFile;
    }
    sealed_file.close();

    if (!boost::filesystem::create_directory(response.paths.cache)) {
      if (boost::filesystem::exists(response.paths.cache)) {
        boost::system::error_code ec;
        boost::filesystem::remove_all(response.paths.cache, ec);
        if (ec.failed()) {
          return WorkerErrors::kCannotRemoveCacheDir;
        }
        if (!boost::filesystem::create_directory(response.paths.cache)) {
          return WorkerErrors::kCannotCreateCacheDir;
        }
      } else {
        return WorkerErrors::kCannotCreateCacheDir;
      }
    }

    UnpaddedPieceSize sum;
    for (const auto &piece : pieces) {
      sum += piece.size.unpadded();
    }

    OUTCOME_TRY(size,
                primitives::sector::getSectorSize(config_.seal_proof_type));

    if (sum != primitives::piece::PaddedPieceSize(size).unpadded()) {
      return WorkerErrors::kPiecesDoNotMatchSectorSize;
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
      const SectorId &sector,
      const sector_storage::PreCommit1Output &pre_commit_1_output) {
    OUTCOME_TRY(response,
                remote_store_->acquireSector(
                    sector,
                    config_.seal_proof_type,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTNone,
                    true));

    return proofs::Proofs::sealPreCommitPhase2(
        pre_commit_1_output, response.paths.cache, response.paths.sealed);
  }

  outcome::result<sector_storage::Commit1Output>
  sector_storage::LocalWorker::sealCommit1(
      const SectorId &sector,
      const primitives::sector::SealRandomness &ticket,
      const primitives::sector::InteractiveRandomness &seed,
      gsl::span<const PieceInfo> pieces,
      const sector_storage::SectorCids &cids) {
    OUTCOME_TRY(response,
                remote_store_->acquireSector(
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
      const SectorId &sector,
      const sector_storage::Commit1Output &commit_1_output) {
    return proofs::Proofs::sealCommitPhase2(
        commit_1_output, sector.sector, sector.miner);
  }

  outcome::result<void> sector_storage::LocalWorker::finalizeSector(
      const SectorId &sector) {
    OUTCOME_TRY(size,
                primitives::sector::getSectorSize(config_.seal_proof_type));

    OUTCOME_TRY(response,
                remote_store_->acquireSector(sector,
                                             config_.seal_proof_type,
                                             SectorFileType::FTCache,
                                             SectorFileType::FTNone,
                                             false));

    OUTCOME_TRY(proofs::Proofs::clearCache(size, response.paths.cache));

    return remote_store_->remove(sector, SectorFileType::FTUnsealed);
  }

  outcome::result<void> sector_storage::LocalWorker::moveStorage(
      const SectorId &sector) {
    return remote_store_->moveStorage(
        sector,
        config_.seal_proof_type,
        static_cast<SectorFileType>(SectorFileType::FTCache
                                    | SectorFileType::FTSealed));
  }

  outcome::result<void> sector_storage::LocalWorker::fetch(
      const SectorId &sector,
      const primitives::sector_file::SectorFileType &file_type,
      bool can_seal) {
    OUTCOME_TRY(remote_store_->acquireSector(sector,
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
      const CID &unsealed_cid) {
    auto maybe_unseal_file =
        remote_store_->acquireSector(sector,
                                     config_.seal_proof_type,
                                     SectorFileType::FTUnsealed,
                                     SectorFileType::FTNone,
                                     false);
    if (maybe_unseal_file.has_error()) {
      if (maybe_unseal_file
          != outcome::failure(
              stores::StoreErrors::kNotFoundRequestedSectorType)) {
        return maybe_unseal_file.error();
      }
      maybe_unseal_file =
          remote_store_->acquireSector(sector,
                                       config_.seal_proof_type,
                                       SectorFileType::FTNone,
                                       SectorFileType::FTUnsealed,
                                       false);
      if (maybe_unseal_file.has_error()) {
        return maybe_unseal_file.error();
      }
    }

    OUTCOME_TRY(response,
                remote_store_->acquireSector(
                    sector,
                    config_.seal_proof_type,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTNone,
                    false));

    boost::filesystem::path temp_file_path =
        boost::filesystem::temp_directory_path()
        / boost::filesystem::unique_path();

    std::ofstream temp_file(temp_file_path.string());
    if (!temp_file.good()) {
      return WorkerErrors::kCannotCreateTempFile;
    }
    temp_file.close();

    auto _ = gsl::finally([&]() {
      boost::system::error_code ec;
      boost::filesystem::remove(temp_file_path, ec);
      if (ec.failed()) {
        logger_->warn("Unable remove temp file: " + ec.message());
      }
    });

    OUTCOME_TRY(
        proofs::Proofs::unsealRange(config_.seal_proof_type,
                                    response.paths.cache,
                                    response.paths.sealed,
                                    temp_file_path.string(),
                                    sector.sector,
                                    sector.miner,
                                    randomness,
                                    unsealed_cid,
                                    primitives::piece::paddedIndex(offset),
                                    size.padded()));

    return proofs::Proofs::writeUnsealPiece(
        temp_file_path.string(),
        maybe_unseal_file.value().paths.unsealed,
        config_.seal_proof_type,
        PaddedPieceSize(primitives::piece::paddedIndex(offset)),
        size);
  }

  outcome::result<void> sector_storage::LocalWorker::readPiece(
      proofs::PieceData output,
      const SectorId &sector,
      primitives::piece::UnpaddedByteIndex offset,
      const primitives::piece::UnpaddedPieceSize &size) {
    OUTCOME_TRY(response,
                remote_store_->acquireSector(sector,
                                             config_.seal_proof_type,
                                             SectorFileType::FTUnsealed,
                                             SectorFileType::FTNone,
                                             false));

    return proofs::Proofs::readPiece(
        std::move(output),
        response.paths.unsealed,
        primitives::piece::PaddedPieceSize(
            primitives::piece::paddedIndex(offset)),
        size);
  }

  outcome::result<primitives::WorkerInfo>
  sector_storage::LocalWorker::getInfo() {
    primitives::WorkerInfo result;

#if __APPLE__
    size_t memorySize = sizeof(int64_t);
    int64_t memory;
    sysctlbyname("hw.memsize", (void *)&memory, &memorySize, nullptr, 0);

    result.resources.physical_memory = memory;

    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    struct vm_statistics64 vm_stat {};
    if (host_statistics64(host_t(mach_host_self()),
                          HOST_VM_INFO64,
                          (host_info_t)(&vm_stat),
                          &count)
        != KERN_SUCCESS) {
      return WorkerErrors::kCannotGetVMStat;
    }

    uint64_t page_size;
    if (host_page_size(host_t(mach_host_self()), (vm_size_t *)(&page_size))
        != KERN_SUCCESS) {
      return WorkerErrors::kCannotGetPageSize;
    }

    uint64_t available_memory =
        (vm_stat.free_count + vm_stat.inactive_count + vm_stat.purgeable_count)
        * page_size;

    xsw_usage vmusage{};
    size_t size = sizeof(vmusage);
    sysctlbyname("vm.swapusage", &vmusage, &size, nullptr, 0);

    result.resources.swap_memory = vmusage.xsu_total;
    result.resources.reserved_memory =
        vmusage.xsu_used + memory - available_memory;
#elif __linux__
    static const std::string memory_file_path = "/proc/meminfo";
    std::ifstream memory_file(memory_file_path);
    if (!memory_file.good()) {
      return WorkerErrors::kCannotOpenMemInfoFile;
    }

    struct {
      uint64_t total;
      uint64_t used;
      uint64_t available;
      uint64_t free;
      uint64_t virtual_total;
      uint64_t virtual_used;
      uint64_t virtual_free;
      std::unordered_map<std::string, uint64_t> metrics;
    } mem_info{};

    bool has_available = false;
    std::string key;
    uint64_t value = 0;
    std::string kb;

    while (std::getline(memory_file, key, ':')) {
      memory_file >> value;
      std::getline(memory_file, kb, '\n');

      if (kb.find("kB") != std::string::npos) {
        value *= 1024;
      }

      if (key == "MemTotal") {
        mem_info.total = value;
        result.resources.physical_memory = value;
      } else if (key == "MemAvailable") {
        has_available = true;
        mem_info.available = value;
      } else if (key == "MemFree") {
        mem_info.free = value;
      } else if (key == "SwapTotal") {
        mem_info.virtual_total = value;
        result.resources.swap_memory = value;
      } else if (key == "SwapFree") {
        mem_info.virtual_free = value;
      } else {
        mem_info.metrics[key] = value;
      }
    }

    mem_info.used = mem_info.total - mem_info.free;
    mem_info.virtual_used = mem_info.virtual_total - mem_info.virtual_free;

    if (!has_available) {
      mem_info.available = mem_info.free + mem_info.metrics["Buffers"]
                           + mem_info.metrics["Cached"];
    }

    result.resources.reserved_memory =
        mem_info.virtual_used + mem_info.total - mem_info.available;
#else
    return WorkerErrors::kUnsupportedPlatform;
#endif

    std::string hostname = config_.hostname;
    if (hostname.empty()) {
      config_.hostname = boost::asio::ip::host_name();
      hostname = config_.hostname;
    }

    result.hostname = hostname;

    result.resources.cpus = std::thread::hardware_concurrency();

    if (result.resources.cpus == 0) {
      return WorkerErrors::kCannotGetNumberOfCPUs;
    }

    OUTCOME_TRYA(result.resources.gpus, proofs::Proofs::getGPUDevices());

    return std::move(result);
  }

  outcome::result<std::set<primitives::TaskType>>
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

    auto cache_err = remote_store_->remove(sector, SectorFileType::FTCache);
    if (cache_err.has_error()) {
      isError = true;
      logger_->error("removing cached sector {} : {}",
                     primitives::sector_file::sectorName(sector),
                     cache_err.error().message());
    }

    auto sealed_err = remote_store_->remove(sector, SectorFileType::FTSealed);
    if (sealed_err.has_error()) {
      isError = true;
      logger_->error("removing sealed sector {} : {}",
                     primitives::sector_file::sectorName(sector),
                     sealed_err.error().message());
    }

    auto unsealed_err =
        remote_store_->remove(sector, SectorFileType::FTUnsealed);
    if (unsealed_err.has_error()) {
      isError = true;
      logger_->error("removing unsealed sector {} : {}",
                     primitives::sector_file::sectorName(sector),
                     unsealed_err.error().message());
    }

    if (isError) {
      return WorkerErrors::kCannotRemoveSector;
    }
    return outcome::success();
  }

  outcome::result<PieceInfo> sector_storage::LocalWorker::addPiece(
      const SectorId &sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      const primitives::piece::UnpaddedPieceSize &new_piece_size,
      const primitives::piece::PieceData &piece_data) {
    if (piece_sizes.empty()) {
      OUTCOME_TRY(response,
                  remote_store_->acquireSector(sector,
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
          response.storages.unsealed, sector, SectorFileType::FTUnsealed));

      return PieceInfo{.size = new_piece_size.padded(),
                       .cid = write_response.piece_cid};
    }

    OUTCOME_TRY(response,
                remote_store_->acquireSector(sector,
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
