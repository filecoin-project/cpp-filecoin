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
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <thread>
#include <utility>
#include "api/storage_miner/storage_api.hpp"
#include "common/put_in_function.hpp"
#include "primitives/rle_bitset/runs_utils.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fc::sector_storage {
  using common::PutInFunction;
  using primitives::piece::PaddedByteIndex;
  using primitives::piece::PaddedPieceSize;
  using primitives::sector_file::SectorFile;
  using primitives::sector_file::SectorFileError;
  using primitives::sector_file::SectorFileTypeErrors;
  using proofs::PieceData;
  namespace uuids = boost::uuids;

  using Self = std::shared_ptr<LocalWorker>;

  template <typename T, typename R>
  bool returnFunction(const CallId &call_id,
                      // can't move because of retry
                      const outcome::result<T> &return_value,
                      const R &_return) {
    while (true) {
      boost::optional<CallError> error;
      if (!return_value) {
        const auto &e{return_value.error()};
        error.emplace(CallError{
            e == StoreError::kCannotReserve ? CallErrorCode::kAllocateSpace
                                            : CallErrorCode::kUnknown,
            e.message(),
        });
      }
      outcome::result<void> maybe_error{outcome::success()};
      if constexpr (std::is_same_v<T, void>) {
        maybe_error = _return(call_id, std::move(error));
      } else {
        T value{};
        if (return_value) {
          value = return_value.value();
        }
        maybe_error = _return(call_id, std::move(value), std::move(error));
      }

      if (not maybe_error.has_error()) break;

      // TODO: wait some time
      // scenario can be that we cannot return, when manager is down.
    }
    return true;
  }

  outcome::result<LocalWorker::Response> LocalWorker::acquireSector(
      SectorRef sector,
      SectorFileType exisitng,
      SectorFileType allocate,
      PathType path,
      AcquireMode mode) {
    OUTCOME_TRY(
        sector_meta,
        remote_store_->acquireSector(sector, exisitng, allocate, path, mode));

    OUTCOME_TRY(
        release_storage,
        remote_store_->getLocalStore()->reserve(
            sector, allocate, sector_meta.storages, PathType::kSealing));

    return LocalWorker::Response{
        .paths = sector_meta.paths,
        .release_function =
            [this,
             allocate,
             storages = std::move(sector_meta.storages),
             release = std::move(release_storage),
             sector_id = sector.id,
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

  LocalWorker::LocalWorker(std::shared_ptr<boost::asio::io_context> context,
                           const WorkerConfig &config,
                           std::shared_ptr<WorkerReturn> return_interface,
                           std::shared_ptr<stores::RemoteStore> store,
                           std::shared_ptr<proofs::ProofEngine> proofs)
      : context_(std::move(context)),
        remote_store_(std::move(store)),
        index_(remote_store_->getSectorIndex()),
        proofs_(std::move(proofs)),
        return_(std::move(return_interface)),
        logger_(common::createLogger("local worker")) {
    if (config.custom_hostname.has_value()) {
      hostname_ = config.custom_hostname.value();
    } else {
      hostname_ = boost::asio::ip::host_name();
    }

    is_no_swap = config.is_no_swap;

    task_types_ = config.task_types;
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

    if (is_no_swap) {
      result.resources.swap_memory = 0;
    }

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
    return task_types_;
  }

  outcome::result<std::vector<primitives::StoragePath>>
  sector_storage::LocalWorker::getAccessiblePaths() {
    return remote_store_->getLocalStore()->getAccessiblePaths();
  }

  outcome::result<CallId> LocalWorker::addPiece(
      const SectorRef &sector,
      VectorCoW<UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      proofs::PieceData piece_data) {
    return asyncCall(
        sector,
        return_->ReturnAddPiece,
        PutInFunction([=,
                       exist_sizes = std::move(piece_sizes),
                       data = std::move(piece_data)](
                          const Self &self) -> outcome::result<PieceInfo> {
          OUTCOME_TRY(max_size,
                      primitives::sector::getSectorSize(sector.proof_type));

          UnpaddedPieceSize offset;

          for (const auto &piece_size : exist_sizes.span()) {
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

          if (exist_sizes.empty()) {
            OUTCOME_TRYA(acquire_response,
                         self->acquireSector(sector,
                                             SectorFileType::FTNone,
                                             SectorFileType::FTUnsealed,
                                             PathType::kSealing));

            OUTCOME_TRYA(staged_file,
                         SectorFile::createFile(acquire_response.paths.unsealed,
                                                PaddedPieceSize(max_size)));
          } else {
            OUTCOME_TRYA(acquire_response,
                         self->acquireSector(sector,
                                             SectorFileType::FTUnsealed,
                                             SectorFileType::FTNone,
                                             PathType::kSealing));

            OUTCOME_TRYA(staged_file,
                         SectorFile::openFile(acquire_response.paths.unsealed,
                                              PaddedPieceSize(max_size)));
          }

          OUTCOME_TRY(piece_info,
                      staged_file->write(data,
                                         offset.padded(),
                                         new_piece_size.padded(),
                                         sector.proof_type));

          return piece_info.value();
        }));
  }

  outcome::result<CallId> LocalWorker::sealPreCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      const std::vector<PieceInfo> &pieces) {
    return asyncCall(
        sector,
        return_->ReturnSealPreCommit1,
        [=](Self self) -> outcome::result<PreCommit1Output> {
          OUTCOME_TRY(
              self->remote_store_->remove(sector.id, SectorFileType::FTSealed));
          OUTCOME_TRY(
              self->remote_store_->remove(sector.id, SectorFileType::FTCache));

          OUTCOME_TRY(response,
                      self->acquireSector(sector,
                                          SectorFileType::FTUnsealed,
                                          static_cast<SectorFileType>(
                                              SectorFileType::FTSealed
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
                      primitives::sector::getSectorSize(sector.proof_type));

          if (sum != primitives::piece::PaddedPieceSize(size).unpadded()) {
            return WorkerErrors::kPiecesDoNotMatchSectorSize;
          }

          return self->proofs_->sealPreCommitPhase1(sector.proof_type,
                                                    response.paths.cache,
                                                    response.paths.unsealed,
                                                    response.paths.sealed,
                                                    sector.id.sector,
                                                    sector.id.miner,
                                                    ticket,
                                                    pieces);
        });
  }

  outcome::result<CallId> LocalWorker::sealPreCommit2(
      const SectorRef &sector, const PreCommit1Output &pre_commit_1_output) {
    return asyncCall(
        sector,
        return_->ReturnSealPreCommit2,
        [=](Self self) -> outcome::result<SectorCids> {
          OUTCOME_TRY(response,
                      self->acquireSector(sector,
                                          static_cast<SectorFileType>(
                                              SectorFileType::FTSealed
                                              | SectorFileType::FTCache),
                                          SectorFileType::FTNone,
                                          PathType::kSealing));
          auto _ = gsl::finally([&]() { response.release_function(); });

          return self->proofs_->sealPreCommitPhase2(
              pre_commit_1_output, response.paths.cache, response.paths.sealed);
        });
  }

  outcome::result<CallId> LocalWorker::sealCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      const InteractiveRandomness &seed,
      const std::vector<PieceInfo> &pieces,
      const SectorCids &cids) {
    return asyncCall(
        sector,
        return_->ReturnSealCommit1,
        [=](Self self) -> outcome::result<Commit1Output> {
          OUTCOME_TRY(response,
                      self->acquireSector(sector,
                                          static_cast<SectorFileType>(
                                              SectorFileType::FTSealed
                                              | SectorFileType::FTCache),
                                          SectorFileType::FTNone,
                                          PathType::kSealing));
          auto _ = gsl::finally([&]() { response.release_function(); });

          return self->proofs_->sealCommitPhase1(sector.proof_type,
                                                 cids.sealed_cid,
                                                 cids.unsealed_cid,
                                                 response.paths.cache,
                                                 response.paths.sealed,
                                                 sector.id.sector,
                                                 sector.id.miner,
                                                 ticket,
                                                 seed,
                                                 pieces);
        });
  }

  outcome::result<CallId> LocalWorker::sealCommit2(
      const SectorRef &sector, const Commit1Output &commit_1_output) {
    return asyncCall(sector,
                     return_->ReturnSealCommit2,
                     [=](Self self) -> outcome::result<Proof> {
                       return self->proofs_->sealCommitPhase2(
                           commit_1_output, sector.id.sector, sector.id.miner);
                     });
  }

  outcome::result<CallId> LocalWorker::finalizeSector(
      const SectorRef &sector, std::vector<Range> keep_unsealed) {
    return asyncCall(
        sector,
        return_->ReturnFinalizeSector,
        [=, keep_unsealed{std::move(keep_unsealed)}](
            Self self) -> outcome::result<void> {
          OUTCOME_TRY(size,
                      primitives::sector::getSectorSize(sector.proof_type));
          {
            if (not keep_unsealed.empty()) {
              std::vector<uint64_t> rle = {0, size};

              for (const auto &sector_info : keep_unsealed) {
                std::vector<uint64_t> sector_rle = {sector_info.offset.padded(),
                                                    sector_info.size.padded()};

                rle = primitives::runsAnd(rle, sector_rle, true);
              }

              OUTCOME_TRY(response,
                          self->acquireSector(sector,
                                              SectorFileType::FTUnsealed,
                                              SectorFileType::FTNone,
                                              PathType::kStorage));
              auto _ = gsl::finally([&]() { response.release_function(); });

              auto maybe_file = SectorFile::openFile(response.paths.unsealed,
                                                     PaddedPieceSize(size));

              if (maybe_file.has_error()) {
                if (maybe_file
                    != outcome::failure(SectorFileError::kFileNotExist)) {
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
                        self->acquireSector(sector,
                                            SectorFileType::FTCache,
                                            SectorFileType::FTNone,
                                            PathType::kStorage));
            auto _ = gsl::finally([&]() { response.release_function(); });

            OUTCOME_TRY(self->proofs_->clearCache(size, response.paths.cache));
          }

          if (keep_unsealed.empty()) {
            OUTCOME_TRY(self->remote_store_->remove(
                sector.id, SectorFileType::FTUnsealed));
          }

          return outcome::success();
        });
  }

  outcome::result<CallId> LocalWorker::replicaUpdate(
      const SectorRef &sector, const std::vector<PieceInfo> &pieces) {
    return asyncCall(
        sector,
        return_->ReturnReplicaUpdate,
        [=](Self self) -> outcome::result<SectorCids> {
          OUTCOME_TRY(
              response,
              self->acquireSector(
                  sector,
                  SectorFileType::FTUnsealed | SectorFileType::FTSealed
                      | SectorFileType::FTCache,
                  SectorFileType::FTUpdate | SectorFileType::FTUpdateCache,
                  PathType::kSealing));
          auto _ = gsl::finally([&]() { response.release_function(); });

          const auto &paths{response.paths};

          OUTCOME_TRY(update_proof_type,
                      getRegisteredUpdateProof(sector.proof_type));

          boost::system::error_code ec;
          auto size = boost::filesystem::file_size(paths.sealed, ec);
          if (ec.failed()) {
            logger_->error("Cannot get file size: {}", ec.message());
            return ERROR_TEXT("Cannot get file size");
          }

          int fd = open(paths.update.c_str(), O_RDWR | O_CREAT, 0644);
          if (fd == -1) {
            return ERROR_TEXT("Cannot create update file");
          }
          close(fd);

          boost::filesystem::resize_file(paths.update, size, ec);
          if (ec.failed()) {
            logger_->error("Cannot resize update file: {}", ec.message());
            return ERROR_TEXT("Cannot resize update file");
          }

          if (!boost::filesystem::create_directory(paths.update_cache)) {
            if (boost::filesystem::exists(paths.update_cache)) {
              boost::filesystem::remove_all(paths.update_cache, ec);
              if (ec.failed()) {
                logger_->error("Cannot create cache update dir: {}",
                               ec.message());
                return ERROR_TEXT("Cannot create cache update dir");
              }
              if (!boost::filesystem::create_directory(response.paths.cache)) {
                logger_->error("Cannot create cache update dir: {}",
                               ec.message());
                return ERROR_TEXT("Cannot create cache update dir");
              }
            } else {
              logger_->error("Cannot create cache update dir: {}",
                             ec.message());
              return ERROR_TEXT("Cannot create cache update dir");
            }
          }

          return self->proofs_->updateSeal(update_proof_type,
                                           paths.update,
                                           paths.update_cache,
                                           paths.sealed,
                                           paths.cache,
                                           paths.unsealed,
                                           gsl::span<const PieceInfo>(pieces));
        });
  }

  outcome::result<CallId> LocalWorker::proveReplicaUpdate1(
      const SectorRef &sector,
      const CID &sector_key,
      const CID &new_sealed,
      const CID &new_unsealed) {
    return asyncCall(
        sector,
        return_->ReturnProveReplicaUpdate1,
        [=](Self self) -> outcome::result<Update1Output> {
          OUTCOME_TRY(response,
                      self->acquireSector(sector,
                                          SectorFileType::FTSealed
                                              | SectorFileType::FTCache
                                              | SectorFileType::FTUpdate
                                              | SectorFileType::FTUpdateCache,
                                          SectorFileType::FTNone,
                                          PathType::kSealing));
          auto _ = gsl::finally([&]() { response.release_function(); });

          const auto &paths{response.paths};

          OUTCOME_TRY(update_proof_type,
                      getRegisteredUpdateProof(sector.proof_type));

          return self->proofs_->updateProve1(update_proof_type,
                                             sector_key,
                                             new_sealed,
                                             new_unsealed,
                                             paths.update,
                                             paths.update_cache,
                                             paths.sealed,
                                             paths.cache);
        });
  }

  outcome::result<CallId> LocalWorker::proveReplicaUpdate2(
      const SectorRef &sector,
      const CID &sector_key,
      const CID &new_sealed,
      const CID &new_unsealed,
      const Update1Output &update_1_output) {
    return asyncCall(sector,
                     return_->ReturnProveReplicaUpdate2,
                     [=](Self self) -> outcome::result<Proof> {
                       OUTCOME_TRY(update_proof_type,
                                   getRegisteredUpdateProof(sector.proof_type));

                       return self->proofs_->updateProve2(update_proof_type,
                                                          sector_key,
                                                          new_sealed,
                                                          new_unsealed,
                                                          update_1_output);
                     });
  }

  outcome::result<CallId> LocalWorker::moveStorage(const SectorRef &sector,
                                                   SectorFileType types) {
    return asyncCall(sector,
                     return_->ReturnMoveStorage,
                     [=](Self self) -> outcome::result<void> {
                       return self->remote_store_->moveStorage(sector, types);
                     });
  }

  outcome::result<CallId> LocalWorker::unsealPiece(
      const SectorRef &sector,
      UnpaddedByteIndex offset,
      const UnpaddedPieceSize &size,
      const SealRandomness &randomness,
      const CID &unsealed_cid) {
    return asyncCall(
        sector,
        return_->ReturnUnsealPiece,
        [=](Self self) -> outcome::result<void> {
          {
            OUTCOME_TRY(sector_size,
                        primitives::sector::getSectorSize(sector.proof_type));

            const PaddedPieceSize max_piece_size(sector_size);

            Response unseal_response;
            auto _ = gsl::final_action([&]() {
              if (unseal_response.release_function) {
                unseal_response.release_function();
              }
            });

            auto maybe_response =
                self->acquireSector(sector,
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
                           self->acquireSector(sector,
                                               SectorFileType::FTNone,
                                               SectorFileType::FTUnsealed,
                                               PathType::kStorage));

              OUTCOME_TRYA(file,
                           SectorFile::createFile(
                               unseal_response.paths.unsealed, max_piece_size))
            } else {
              unseal_response = maybe_response.value();
              OUTCOME_TRYA(file,
                           SectorFile::openFile(unseal_response.paths.unsealed,
                                                max_piece_size))
            }

            auto computeUnsealRanges =
                [file](PaddedByteIndex offset,
                       PaddedPieceSize size) -> std::vector<Range> {
              constexpr uint64_t kMergeGaps =
                  uint64_t(32) << 20;  // TODO: find optimal number

              auto rle = file->allocated();
              auto to_unsealed = primitives::runsAnd(
                  std::vector<uint64_t>{offset, size}, rle, true);

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

            OUTCOME_TRY(response,
                        self->acquireSector(sector,
                                            static_cast<SectorFileType>(
                                                SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                                            SectorFileType::FTNone,
                                            PathType::kStorage));
            auto _1 = gsl::final_action([&]() { response.release_function(); });

            PieceData sealed{response.paths.sealed, O_RDONLY};

            if (not sealed.isOpened()) {
              return WorkerErrors::kCannotOpenFile;
            }

            std::vector<std::thread> threads;
            for (const auto &range : to_unsealed) {
              int fds[2];
              if (pipe(fds) < 0) {
                return ERROR_TEXT("Unseal: cannot open pipe");
              }

              threads.emplace_back([reader = PieceData(fds[0]),
                                    file,
                                    range,
                                    logger{self->logger_}]() {
                auto maybe_error{
                    file->write(reader,
                                primitives::piece::paddedIndex(range.offset),
                                range.size.padded())};
                if (maybe_error.has_error()) {
                  logger->error("ERROR: {}", maybe_error.error().message());
                }
              });

              OUTCOME_TRY(self->proofs_->unsealRange(sector.proof_type,
                                                     response.paths.cache,
                                                     sealed,
                                                     PieceData(fds[1]),
                                                     sector.id.sector,
                                                     sector.id.miner,
                                                     randomness,
                                                     unsealed_cid,
                                                     range.offset,
                                                     range.size));
            }

            for (auto &t : threads) {
              if (t.joinable()) {
                t.join();
              }
            }
          }

          OUTCOME_TRY(self->remote_store_->removeCopies(
              sector.id, SectorFileType::FTSealed));

          OUTCOME_TRY(self->remote_store_->removeCopies(
              sector.id, SectorFileType::FTCache));

          return outcome::success();
        });
  }

  outcome::result<CallId> LocalWorker::readPiece(
      PieceData output,
      const SectorRef &sector,
      UnpaddedByteIndex offset,
      const UnpaddedPieceSize &size) {
    return asyncCall(
        sector,
        return_->ReturnReadPiece,
        [=, output = std::make_shared<PieceData>(std::move(output))](
            Self self) -> outcome::result<bool> {
          OUTCOME_TRY(response,
                      self->acquireSector(sector,
                                          SectorFileType::FTUnsealed,
                                          SectorFileType::FTNone,
                                          PathType::kStorage));
          auto _ = gsl::final_action([&]() { response.release_function(); });

          OUTCOME_TRY(sector_size,
                      primitives::sector::getSectorSize(sector.proof_type));

          const PaddedPieceSize max_piece_size(sector_size);

          auto maybe_file =
              SectorFile::openFile(response.paths.unsealed, max_piece_size);

          if (maybe_file.has_error()) {
            if (maybe_file
                == outcome::failure(SectorFileError::kFileNotExist)) {
              return false;
            }

            return maybe_file.error();
          }

          auto &file{maybe_file.value()};

          OUTCOME_TRY(is_allocated, file->hasAllocated(offset, size));

          if (not is_allocated) {
            return false;
          }
          return file->read(std::move(*output),
                            primitives::piece::paddedIndex(offset),
                            size.padded());
        });
  }

  outcome::result<CallId> LocalWorker::fetch(const SectorRef &sector,
                                             const SectorFileType &file_type,
                                             PathType path_type,
                                             AcquireMode mode) {
    return asyncCall(
        sector, return_->ReturnFetch, [=](Self self) -> outcome::result<void> {
          OUTCOME_TRY(
              res,
              self->acquireSector(
                  sector, file_type, SectorFileType::FTNone, path_type, mode));
          res.release_function();
          return outcome::success();
        });
  }

  template <typename W, typename R>
  outcome::result<CallId> LocalWorker::asyncCall(const SectorRef &sector,
                                                 R _return,
                                                 W work) {
    CallId call_id{.sector = sector.id,
                   .id = uuids::to_string(uuids::random_generator()())};

    context_->post([self{shared_from_this()},
                    call_id,
                    work{std::move(work)},
                    _return{std::move(_return)}] {
      returnFunction(call_id, work(self), _return);
    });

    return call_id;
  }
  void LocalWorker::ping(std::function<void(const bool resp)> cb) {
    cb(true);
  }
}  // namespace fc::sector_storage
