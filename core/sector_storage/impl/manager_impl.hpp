/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/manager.hpp"

#include <boost/asio/io_context.hpp>
#include <future>
#include "common/error_text.hpp"
#include "proofs/impl/proof_engine_impl.hpp"
#include "sector_storage/scheduler.hpp"
#include "sector_storage/stores/index.hpp"
#include "sector_storage/stores/store.hpp"

namespace fc::sector_storage {
  using primitives::SectorNumber;
  using primitives::sector::RegisteredPoStProof;

  struct SealerConfig {
    bool allow_precommit_1;
    bool allow_precommit_2;
    bool allow_commit;
    bool allow_unseal;
  };

  class ManagerImpl : public Manager,
                      public std::enable_shared_from_this<ManagerImpl> {
   public:
    static outcome::result<std::shared_ptr<Manager>> newManager(
        const std::shared_ptr<boost::asio::io_context> &io_context,
        const std::shared_ptr<stores::RemoteStore> &remote,
        const std::shared_ptr<Scheduler> &scheduler,
        const SealerConfig &config,
        const std::shared_ptr<proofs::ProofEngine> &proofs =
            std::make_shared<proofs::ProofEngineImpl>());

    void readPiece(PieceData output,
                   const SectorRef &sector,
                   UnpaddedByteIndex offset,
                   const UnpaddedPieceSize &size,
                   const SealRandomness &randomness,
                   const CID &cid,
                   const std::function<void(outcome::result<bool>)> &cb,
                   uint64_t priority) override;

    void sealPreCommit1(
        const SectorRef &sector,
        const SealRandomness &ticket,
        const std::vector<PieceInfo> &pieces,
        const std::function<void(outcome::result<PreCommit1Output>)> &cb,
        uint64_t priority) override;

    void sealPreCommit2(
        const SectorRef &sector,
        const PreCommit1Output &pre_commit_1_output,
        const std::function<void(outcome::result<SectorCids>)> &cb,
        uint64_t priority) override;

    void sealCommit1(
        const SectorRef &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        const std::vector<PieceInfo> &pieces,
        const SectorCids &cids,
        const std::function<void(outcome::result<Commit1Output>)> &cb,
        uint64_t priority) override;

    void sealCommit2(const SectorRef &sector,
                     const Commit1Output &commit_1_output,
                     const std::function<void(outcome::result<Proof>)> &cb,
                     uint64_t priority) override;

    void finalizeSector(const SectorRef &sector,
                        std::vector<Range> keep_unsealed,
                        const std::function<void(outcome::result<void>)> &cb,
                        uint64_t priority) override;

    void replicaUpdate(
        const SectorRef &sector,
        const std::vector<PieceInfo> &pieces,
        const std::function<void(outcome::result<ReplicaUpdateOut>)> &cb,
        uint64_t priority) override;

    void proveReplicaUpdate1(
        const SectorRef &sector,
        const CID &sector_key,
        const CID &new_sealed,
        const CID &new_unsealed,
        const std::function<void(outcome::result<ReplicaVanillaProofs>)> &cb,
        uint64_t priority) override;

    void proveReplicaUpdate2(
        const SectorRef &sector,
        const CID &sector_key,
        const CID &new_sealed,
        const CID &new_unsealed,
        const Update1Output &update_1_output,
        const std::function<void(outcome::result<ReplicaUpdateProof>)> &cb,
        uint64_t priority) override;

    void finalizeReplicaUpdate(
        const SectorRef &sector,
        std::vector<Range> keep_unsealed,
        const std::function<void(outcome::result<void>)> &cb,
        uint64_t priority) override;

    outcome::result<void> releaseReplicaUpgrade(
        const SectorRef &sector) override;
    outcome::result<void> releaseSectorKey(const SectorRef &sector) override;

    void addPiece(const SectorRef &sector,
                  VectorCoW<UnpaddedPieceSize> piece_sizes,
                  const UnpaddedPieceSize &new_piece_size,
                  proofs::PieceData piece_data,
                  const std::function<void(outcome::result<PieceInfo>)> &cb,
                  uint64_t priority) override;

    outcome::result<PieceInfo> addPieceSync(
        const SectorRef &sector,
        VectorCoW<UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        proofs::PieceData piece_data,
        uint64_t priority) override;

    outcome::result<std::vector<SectorId>> checkProvable(
        RegisteredPoStProof proof_type,
        gsl::span<const SectorRef> sectors) const override;

    std::shared_ptr<proofs::ProofEngine> getProofEngine() const override;

    outcome::result<std::vector<PoStProof>> generateWinningPoSt(
        ActorId miner_id,
        gsl::span<const ExtendedSectorInfo> sector_info,
        PoStRandomness randomness) override;

    outcome::result<WindowPoStResponse> generateWindowPoSt(
        ActorId miner_id,
        gsl::span<const ExtendedSectorInfo> sector_info,
        PoStRandomness randomness) override;

    outcome::result<void> remove(const SectorRef &sector) override;

    outcome::result<void> addLocalStorage(const std::string &path) override;

    outcome::result<void> addWorker(std::shared_ptr<Worker> worker) override;

    outcome::result<std::unordered_map<StorageID, std::string>>
    getLocalStorages() override;

    outcome::result<FsStat> getFsStat(StorageID storage_id) override;

   private:
    void finalizeSectorInner(
        const SectorRef &sector,
        std::vector<Range> keep_unsealed,
        SectorFileType main_type,
        SectorFileType additional_types,
        const std::function<void(outcome::result<void>)> &cb,
        uint64_t priority);

    ManagerImpl(std::shared_ptr<stores::SectorIndex> sector_index,
                std::shared_ptr<stores::LocalStorage> local_storage,
                std::shared_ptr<stores::LocalStore> local_store,
                std::shared_ptr<stores::RemoteStore> store,
                std::shared_ptr<Scheduler> scheduler,
                std::shared_ptr<proofs::ProofEngine> proofs);

    template <typename T>
    ReturnCb callbackWrapper(
        const std::function<void(outcome::result<T>)> &cb) {
      return [self{shared_from_this()},
              cb](outcome::result<CallResult> res) -> void {
        static const auto VariantError = ERROR_TEXT("Incorrect return type");
        static const auto CallError = ERROR_TEXT("Call returns error");
        if (res.has_error()) {
          return cb(res.error());
        }

        auto &call_res{res.value()};

        if (call_res.maybe_error) {
          auto &error{call_res.maybe_error.value()};
          self->logger_->error(
              "Call error (" + std::to_string(static_cast<uint64_t>(error.code))
              + "): " + error.message);
          return cb(CallError);
        }

        if (auto *value = std::get_if<T>(&(call_res.value))) {
          return cb(*value);
        }

        return cb(VariantError);
      };
    }

    ReturnCb callbackWrapper(
        const std::function<void(outcome::result<void>)> &cb) {
      return [self{shared_from_this()},
              cb](outcome::result<CallResult> res) -> void {
        auto CallError = ERROR_TEXT("Call returns error");
        if (res.has_error()) {
          return cb(res.error());
        }

        auto &call_res{res.value()};

        if (call_res.maybe_error) {
          auto &error{call_res.maybe_error.value()};
          self->logger_->error(
              "Call error (" + std::to_string(static_cast<uint64_t>(error.code))
              + "): " + error.message);
          return cb(CallError);
        }

        return cb(outcome::success());
      };
    }

    struct Response {
      stores::SectorPaths paths;
      std::shared_ptr<stores::WLock> lock;
    };

    outcome::result<Response> acquireSector(SectorRef sector,
                                            SectorFileType exisitng,
                                            SectorFileType allocate,
                                            PathType path);

    struct PubToPrivateResponse {
      proofs::SortedPrivateSectorInfo private_info;
      std::vector<SectorId> skipped;
      std::vector<std::shared_ptr<stores::WLock>> locks;
    };

    outcome::result<PubToPrivateResponse> publicSectorToPrivate(
        ActorId miner,
        gsl::span<const ExtendedSectorInfo> sector_info,
        bool winning);

    std::shared_ptr<stores::SectorIndex> index_;

    std::shared_ptr<stores::LocalStorage> local_storage_;
    std::shared_ptr<stores::LocalStore> local_store_;
    std::shared_ptr<stores::RemoteStore> remote_store_;

    std::shared_ptr<Scheduler> scheduler_;

    common::Logger logger_;

    std::shared_ptr<proofs::ProofEngine> proofs_;
  };

}  // namespace fc::sector_storage
