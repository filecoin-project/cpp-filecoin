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
#include "primitives/rle_bitset/runs_utils.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fc::sector_storage {
  using primitives::piece::PaddedByteIndex;
  using primitives::piece::PaddedPieceSize;
  using primitives::sector_file::SectorFile;
  using primitives::sector_file::SectorFileError;
  using primitives::sector_file::SectorFileTypeErrors;
  using proofs::PieceData;

  outcome::result<LocalWorker::Response> LocalWorker::acquireSector(
      SectorId sector_id,
      SectorFileType exisitng,
      SectorFileType allocate,
      PathType path,
      AcquireMode mode) {
    OUTCOME_TRY(sector_meta,
                remote_store_->acquireSector(sector_id,
                                             config_.seal_proof_type,
                                             exisitng,
                                             allocate,
                                             path,
                                             mode));

    OUTCOME_TRY(release_storage,
                remote_store_->getLocalStore()->reserve(config_.seal_proof_type,
                                                        allocate,
                                                        sector_meta.storages,
                                                        PathType::kSealing));

    return LocalWorker::Response{
        .paths = sector_meta.paths,
        .release_function =
            [this,
             allocate,
             storages = std::move(sector_meta.storages),
             release = std::move(release_storage),
             sector_id,
             mode]() {
              release();

              for (const auto &type :
                   primitives::sector_file::kSectorFileTypes) {
                if ((type & allocate) == 0) {
                  continue;
                }

                auto maybe_sid = storages.getPathByType(type);
                if (maybe_sid.has_error()) {
                  this->logger_->error(maybe_sid.error().message());
                  continue;
                }

                auto maybe_error =
                    index_->storageDeclareSector(maybe_sid.value(),
                                                 sector_id,
                                                 type,
                                                 mode == AcquireMode::kMove);
                if (maybe_error.has_error()) {
                  this->logger_->error(maybe_error.error().message());
                }
              }
            },
    };
  }

  LocalWorker::LocalWorker(WorkerConfig config,
                           std::shared_ptr<stores::RemoteStore> store,
                           std::shared_ptr<proofs::ProofEngine> proofs)
      : remote_store_(std::move(store)),
        index_(remote_store_->getSectorIndex()),
        proofs_(std::move(proofs)),
        config_(std::move(config)),
        hostname_(boost::asio::ip::host_name()),
        logger_(common::createLogger("local worker")) {}

  outcome::result<sector_storage::PreCommit1Output>
  sector_storage::LocalWorker::sealPreCommit1(
      const SectorId &sector,
      const primitives::sector::SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    OUTCOME_TRY(remote_store_->remove(sector, SectorFileType::FTSealed));
    OUTCOME_TRY(remote_store_->remove(sector, SectorFileType::FTCache));

    OUTCOME_TRY(
        response,
        acquireSector(sector,
                      SectorFileType::FTUnsealed,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      PathType::kSealing));

    auto _ = gsl::finally([&]() { response.release_function(); });

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

    return proofs_->sealPreCommitPhase1(config_.seal_proof_type,
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
    OUTCOME_TRY(
        response,
        acquireSector(sector,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kSealing));
    auto _ = gsl::finally([&]() { response.release_function(); });

    return proofs_->sealPreCommitPhase2(
        pre_commit_1_output, response.paths.cache, response.paths.sealed);
  }

  outcome::result<sector_storage::Commit1Output>
  sector_storage::LocalWorker::sealCommit1(
      const SectorId &sector,
      const primitives::sector::SealRandomness &ticket,
      const primitives::sector::InteractiveRandomness &seed,
      gsl::span<const PieceInfo> pieces,
      const sector_storage::SectorCids &cids) {
    OUTCOME_TRY(
        response,
        acquireSector(sector,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kSealing));
    auto _ = gsl::finally([&]() { response.release_function(); });

    return proofs_->sealCommitPhase1(config_.seal_proof_type,
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
    return proofs_->sealCommitPhase2(
        commit_1_output, sector.sector, sector.miner);
  }

  outcome::result<void> sector_storage::LocalWorker::finalizeSector(
      const SectorId &sector, const gsl::span<const Range> &keep_unsealed) {
    OUTCOME_TRY(size,
                primitives::sector::getSectorSize(config_.seal_proof_type));
    {
      if (not keep_unsealed.empty()) {
        std::vector<uint64_t> rle = {0, size};

        for (const auto &sector_info : keep_unsealed) {
          std::vector<uint64_t> sector_rle = {sector_info.offset.padded(),
                                              sector_info.size.padded()};

          rle = primitives::runsAnd(rle, sector_rle, true);
        }

        OUTCOME_TRY(response,
                    acquireSector(sector,
                                  SectorFileType::FTUnsealed,
                                  SectorFileType::FTNone,
                                  PathType::kStorage));
        auto _ = gsl::finally([&]() { response.release_function(); });

        auto maybe_file = SectorFile::openFile(response.paths.unsealed,
                                               PaddedPieceSize(size));

        if (maybe_file.has_error()) {
          if (maybe_file != outcome::failure(SectorFileError::kFileNotExist)) {
            return maybe_file.error();
          }
        } else {
          auto &file{maybe_file.value()};
          PaddedPieceSize offset(0);
          bool is_value = false;
          for (const auto &elem : rle) {
            if (is_value) {
              OUTCOME_TRY(file->free(offset, PaddedPieceSize(elem)));
            }
            offset += elem;
            is_value = not is_value;
          }
        }
      }
      OUTCOME_TRY(response,
                  acquireSector(sector,
                                SectorFileType::FTCache,
                                SectorFileType::FTNone,
                                PathType::kStorage));
      auto _ = gsl::finally([&]() { response.release_function(); });

      OUTCOME_TRY(proofs_->clearCache(size, response.paths.cache));
    }

    if (keep_unsealed.empty()) {
      OUTCOME_TRY(remote_store_->remove(sector, SectorFileType::FTUnsealed));
    }

    return outcome::success();
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
      PathType path_type,
      AcquireMode mode) {
    OUTCOME_TRY(
        res,
        acquireSector(
            sector, file_type, SectorFileType::FTNone, path_type, mode));
    res.release_function();
    return outcome::success();
  }

  outcome::result<void> sector_storage::LocalWorker::unsealPiece(
      const SectorId &sector,
      primitives::piece::UnpaddedByteIndex offset,
      const primitives::piece::UnpaddedPieceSize &size,
      const primitives::sector::SealRandomness &randomness,
      const CID &unsealed_cid) {
    {
      OUTCOME_TRY(sector_size,
                  primitives::sector::getSectorSize(config_.seal_proof_type));

      PaddedPieceSize max_piece_size(sector_size);

      Response unseal_response;
      auto _ = gsl::final_action([&]() {
        if (unseal_response.release_function) {
          unseal_response.release_function();
        }
      });

      auto maybe_response = acquireSector(sector,
                                          SectorFileType::FTUnsealed,
                                          SectorFileType::FTNone,
                                          PathType::kStorage);
      std::shared_ptr<SectorFile> file;

      if (maybe_response.has_error()) {
        if (maybe_response
            != outcome::failure(stores::StoreError::kNotFoundSector)) {
          return maybe_response.error();
        }
        OUTCOME_TRYA(unseal_response,
                     acquireSector(sector,
                                   SectorFileType::FTNone,
                                   SectorFileType::FTUnsealed,
                                   PathType::kStorage));

        OUTCOME_TRYA(file,
                     SectorFile::createFile(unseal_response.paths.unsealed,
                                            max_piece_size))
      } else {
        unseal_response = maybe_response.value();
        OUTCOME_TRYA(file,
                     SectorFile::openFile(unseal_response.paths.unsealed,
                                          max_piece_size))
      }

      auto computeUnsealRanges =
          [file](PaddedByteIndex offset,
                 PaddedPieceSize size) -> std::vector<Range> {
        static uint64_t kMergeGaps = uint64_t(32)
                                     << 20;  // TODO: find optimal number

        auto rle = file->allocated();
        auto to_unsealed =
            primitives::runsAnd(std::vector<uint64_t>{offset, size}, rle, true);

        if (to_unsealed.size() % 2 != 0) {
          to_unsealed.pop_back();
        }

        if (to_unsealed.empty()) {
          return {};
        }

        PaddedPieceSize amount_offset(to_unsealed[0]);

        std::vector<Range> ranges = {Range{
            .offset = amount_offset.unpadded(),
            .size = PaddedPieceSize(to_unsealed[1]).unpadded(),
        }};

        amount_offset += to_unsealed[1];

        for (uint64_t i = 2; i < to_unsealed.size(); i += 2) {
          amount_offset += to_unsealed[i];
          if (to_unsealed[i] < kMergeGaps) {
            ranges.back().size += (to_unsealed[i] + to_unsealed[i + 1]);
          } else {
            ranges.push_back(Range{
                .offset = amount_offset.unpadded(),
                .size = PaddedPieceSize(to_unsealed[i + 1]).unpadded(),
            });
          }
          amount_offset += to_unsealed[i + 1];
        }

        return ranges;
      };

      std::vector<Range> to_unsealed = computeUnsealRanges(
          primitives::piece::paddedIndex(offset), size.padded());

      if (to_unsealed.empty()) {
        return outcome::success();
      }

      OUTCOME_TRY(
          response,
          acquireSector(sector,
                        static_cast<SectorFileType>(SectorFileType::FTSealed
                                                    | SectorFileType::FTCache),
                        SectorFileType::FTNone,
                        PathType::kStorage));
      auto _1 = gsl::final_action([&]() { response.release_function(); });

      PieceData sealed{response.paths.sealed, O_RDONLY};

      if (not sealed.isOpened()) {
        return WorkerErrors::kCannotOpenFile;
      }

      for (const auto &range : to_unsealed) {
        int fds[2];
        if (pipe(fds) < 0) {
          return outcome::success();
        }

        PieceData reader(fds[0]);

        // TODO: can be in another thread
        OUTCOME_TRY(
            proofs_->unsealRange(config_.seal_proof_type,
                                 response.paths.cache,
                                 sealed,
                                 PieceData(fds[1]),
                                 sector.sector,
                                 sector.miner,
                                 randomness,
                                 unsealed_cid,
                                 primitives::piece::paddedIndex(range.offset),
                                 range.size.padded()));

        OUTCOME_TRY(file->write(reader,
                                primitives::piece::paddedIndex(range.offset),
                                range.size.padded()));
      }
    }

    OUTCOME_TRY(remote_store_->removeCopies(sector, SectorFileType::FTSealed));

    OUTCOME_TRY(remote_store_->removeCopies(sector, SectorFileType::FTCache));

    return outcome::success();
  }

  outcome::result<bool> sector_storage::LocalWorker::readPiece(
      proofs::PieceData output,
      const SectorId &sector,
      primitives::piece::UnpaddedByteIndex offset,
      const primitives::piece::UnpaddedPieceSize &size) {
    OUTCOME_TRY(response,
                acquireSector(sector,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage));
    auto _ = gsl::final_action([&]() { response.release_function(); });

    OUTCOME_TRY(sector_size,
                primitives::sector::getSectorSize(config_.seal_proof_type));

    PaddedPieceSize max_piece_size(sector_size);

    auto maybe_file =
        SectorFile::openFile(response.paths.unsealed, max_piece_size);

    if (maybe_file.has_error()) {
      if (maybe_file == outcome::failure(SectorFileError::kFileNotExist)) {
        return false;
      }

      return maybe_file.error();
    }

    auto &file{maybe_file.value()};

    OUTCOME_TRY(is_allocated, file->hasAllocated(offset, size));

    if (not is_allocated) {
      return false;
    }

    return file->read(
        output, primitives::piece::paddedIndex(offset), size.padded());
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

    result.hostname = hostname_;

    result.resources.cpus = std::thread::hardware_concurrency();

    if (result.resources.cpus == 0) {
      return WorkerErrors::kCannotGetNumberOfCPUs;
    }

    OUTCOME_TRYA(result.resources.gpus, proofs_->getGPUDevices());

    return std::move(result);
  }

  outcome::result<std::set<primitives::TaskType>>
  sector_storage::LocalWorker::getSupportedTask() {
    return config_.task_types;
  }

  outcome::result<std::vector<primitives::StoragePath>>
  sector_storage::LocalWorker::getAccessiblePaths() {
    return remote_store_->getLocalStore()->getAccessiblePaths();
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
    OUTCOME_TRY(max_size,
                primitives::sector::getSectorSize(config_.seal_proof_type));

    UnpaddedPieceSize offset;

    for (const auto &piece_size : piece_sizes) {
      offset += piece_size;
    }

    if ((offset.padded() + new_piece_size.padded()) > max_size) {
      return WorkerErrors::kOutOfBound;
    }

    std::shared_ptr<SectorFile> staged_file;
    Response acquire_response{};
    auto _ = gsl::final_action([&]() {
      if (acquire_response.release_function) {
        acquire_response.release_function();
      }
    });

    if (piece_sizes.empty()) {
      OUTCOME_TRYA(acquire_response,
                   acquireSector(sector,
                                 SectorFileType::FTNone,
                                 SectorFileType::FTUnsealed,
                                 PathType::kSealing));

      OUTCOME_TRYA(staged_file,
                   SectorFile::createFile(acquire_response.paths.unsealed,
                                          PaddedPieceSize(max_size)));
    } else {
      OUTCOME_TRYA(acquire_response,
                   acquireSector(sector,
                                 SectorFileType::FTUnsealed,
                                 SectorFileType::FTNone,
                                 PathType::kSealing));

      OUTCOME_TRYA(staged_file,
                   SectorFile::openFile(acquire_response.paths.unsealed,
                                        PaddedPieceSize(max_size)));
    }

    OUTCOME_TRY(piece_info,
                staged_file->write(piece_data,
                                   offset.padded(),
                                   new_piece_size.padded(),
                                   config_.seal_proof_type));

    return piece_info.value();
  }
}  // namespace fc::sector_storage
