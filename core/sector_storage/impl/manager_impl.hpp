/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_MANAGER_IMPL_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_MANAGER_IMPL_HPP

#include "sector_storage/manager.hpp"

#include "sector_storage/scheduler.hpp"
#include "sector_storage/stores/impl/local_store.hpp"
#include "sector_storage/stores/impl/remote_store.hpp"
#include "sector_storage/stores/index.hpp"
#include "sector_storage/stores/store.hpp"

namespace fc::sector_storage {

  using primitives::SectorNumber;

  struct SealerConfig {
    bool allow_precommit_1;
    bool allow_precommit_2;
    bool allow_commit;
    bool allow_unseal;
  };

  class ManagerImpl : public Manager {
   public:
    static outcome::result<std::unique_ptr<Manager>> newManager(
        const std::shared_ptr<stores::RemoteStore> &remote,
        const std::shared_ptr<Scheduler> &scheduler,
        const SealerConfig &config);

    outcome::result<std::vector<SectorId>> checkProvable(
        RegisteredProof seal_proof_type,
        gsl::span<const SectorId> sectors) override;

    SectorSize getSectorSize() override;

    outcome::result<void> ReadPiece(proofs::PieceData output,
                                    const SectorId &sector,
                                    UnpaddedByteIndex offset,
                                    const UnpaddedPieceSize &size,
                                    const SealRandomness &randomness,
                                    const CID &cid) override;

    outcome::result<std::vector<PoStProof>> generateWinningPoSt(
        ActorId miner_id,
        gsl::span<const SectorInfo> sector_info,
        PoStRandomness randomness) override;

    outcome::result<WindowPoStResponse> generateWindowPoSt(
        ActorId miner_id,
        gsl::span<const SectorInfo> sector_info,
        PoStRandomness randomness) override;

    outcome::result<PreCommit1Output> sealPreCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces) override;

    outcome::result<SectorCids> sealPreCommit2(
        const SectorId &sector,
        const PreCommit1Output &pre_commit_1_output) override;

    outcome::result<Commit1Output> sealCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        gsl::span<const PieceInfo> pieces,
        const SectorCids &cids) override;

    outcome::result<Proof> sealCommit2(
        const SectorId &sector, const Commit1Output &commit_1_output) override;

    outcome::result<void> finalizeSector(const SectorId &sector) override;

    outcome::result<void> remove(const SectorId &sector) override;

    outcome::result<PieceInfo> addPiece(
        const SectorId &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        const proofs::PieceData &piece_data) override;

    outcome::result<void> addLocalStorage(const std::string &path) override;

    outcome::result<void> addWorker(std::shared_ptr<Worker> worker) override;

    outcome::result<std::unordered_map<StorageID, std::string>>
    getLocalStorages() override;

    outcome::result<FsStat> getFsStat(StorageID storage_id) override;

   private:
    ManagerImpl(std::shared_ptr<stores::SectorIndex> sector_index,
                RegisteredProof seal_proof_type,
                std::shared_ptr<stores::LocalStorage> local_storage,
                std::shared_ptr<stores::LocalStore> local_store,
                std::shared_ptr<stores::RemoteStore> store,
                std::shared_ptr<Scheduler> scheduler);

    struct PubToPrivateResponse {
      proofs::SortedPrivateSectorInfo private_info;
      std::vector<SectorId> skipped;
    };

    outcome::result<PubToPrivateResponse> publicSectorToPrivate(
        ActorId miner,
        gsl::span<const SectorInfo> sector_info,
        gsl::span<const SectorNumber> faults,
        const std::function<outcome::result<RegisteredProof>(RegisteredProof)>
            &to_post_transform);

    std::shared_ptr<stores::SectorIndex> index_;

    RegisteredProof seal_proof_type_;  // TODO: maybe add config
    SectorSize sector_size_;

    std::shared_ptr<stores::LocalStorage> local_storage_;
    std::shared_ptr<stores::LocalStore> local_store_;
    std::shared_ptr<stores::RemoteStore> remote_store_;

    std::shared_ptr<Scheduler> scheduler_;

    common::Logger logger_;
  };

}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_MANAGER_IMPL_HPP
