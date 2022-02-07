/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "sector_storage/manager.hpp"

namespace fc::sector_storage {
  class ManagerMock : public Manager {
   public:
    MOCK_CONST_METHOD0(getProofEngine, std::shared_ptr<proofs::ProofEngine>());

    MOCK_CONST_METHOD2(checkProvable,
                       outcome::result<std::vector<SectorId>>(
                           RegisteredPoStProof, gsl::span<const SectorRef>));

    MOCK_METHOD8(readPiece,
                 void(PieceData,
                      const SectorRef &,
                      UnpaddedByteIndex,
                      const UnpaddedPieceSize &,
                      const SealRandomness &,
                      const CID &,
                      const std::function<void(outcome::result<bool>)> &,
                      uint64_t));

    MOCK_METHOD1(addLocalStorage,
                 outcome::result<void>(const std::string &path));

    MOCK_METHOD1(addWorker,
                 outcome::result<void>(std::shared_ptr<Worker> worker));

    MOCK_METHOD0(getLocalStorages,
                 outcome::result<std::unordered_map<StorageID, std::string>>());

    MOCK_METHOD1(getFsStat, outcome::result<FsStat>(StorageID storage_id));

    MOCK_METHOD5(
        sealPreCommit1,
        void(const SectorRef &sector,
             const SealRandomness &ticket,
             const std::vector<PieceInfo> &pieces,
             const std::function<void(outcome::result<PreCommit1Output>)> &,
             uint64_t priority));

    MOCK_METHOD4(
        sealPreCommit2,
        void(const SectorRef &sector,
             const PreCommit1Output &pre_commit_1_output,
             const std::function<void(outcome::result<SectorCids>)> &cb,
             uint64_t priority));

    MOCK_METHOD7(
        sealCommit1,
        void(const SectorRef &sector,
             const SealRandomness &ticket,
             const InteractiveRandomness &seed,
             const std::vector<PieceInfo> &pieces,
             const SectorCids &cids,
             const std::function<void(outcome::result<Commit1Output>)> &,
             uint64_t priority));

    MOCK_METHOD4(sealCommit2,
                 void(const SectorRef &sector,
                      const Commit1Output &commit_1_output,
                      const std::function<void(outcome::result<Proof>)> &,
                      uint64_t priority));

    MOCK_METHOD4(finalizeSector,
                 void(const SectorRef &sector,
                      const gsl::span<const Range> &keep_unsealed,
                      const std::function<void(outcome::result<void>)> &,
                      uint64_t priority));

    MOCK_METHOD1(remove, outcome::result<void>(const SectorRef &sector));

    MOCK_METHOD4(
        replicaUpdate,
        void(const SectorRef &sector,
             const std::vector<PieceInfo> &pieces,
             const std::function<void(outcome::result<ReplicaUpdateOut>)> &cb,
             uint64_t priority));

    MOCK_METHOD6(
        proveReplicaUpdate1,
        void(const SectorRef &sector,
             const CID &sector_key,
             const CID &new_sealed,
             const CID &new_unsealed,
             const std::function<void(outcome::result<ReplicaVanillaProofs>)>
                 &cb,
             uint64_t priority));

    MOCK_METHOD7(
        proveReplicaUpdate2,
        void(const SectorRef &sector,
             const CID &sector_key,
             const CID &new_sealed,
             const CID &new_unsealed,
             const Update1Output &update_1_output,
             const std::function<void(outcome::result<ReplicaUpdateProof>)> &cb,
             uint64_t priority));

    MOCK_METHOD6(doAddPiece,
                 void(const SectorRef &sector,
                      gsl::span<const UnpaddedPieceSize> piece_sizes,
                      const UnpaddedPieceSize &new_piece_size,
                      int,
                      const std::function<void(outcome::result<PieceInfo>)> &,
                      uint64_t priority));

    MOCK_METHOD5(doAddNullPiece,
                 void(const SectorRef &sector,
                      gsl::span<const UnpaddedPieceSize> piece_sizes,
                      const UnpaddedPieceSize &new_piece_size,
                      const std::function<void(outcome::result<PieceInfo>)> &,
                      uint64_t priority));

    void addPiece(const SectorRef &sector,
                  gsl::span<const UnpaddedPieceSize> piece_sizes,
                  const UnpaddedPieceSize &new_piece_size,
                  proofs::PieceData piece_data,
                  const std::function<void(outcome::result<PieceInfo>)> &cb,
                  uint64_t priority) override {
      if (piece_data.isNullData()) {
        return doAddNullPiece(
            sector, piece_sizes, new_piece_size, cb, priority);
      }

      return doAddPiece(sector,
                        piece_sizes,
                        new_piece_size,
                        piece_data.getFd(),
                        cb,
                        priority);
    }

    MOCK_METHOD5(doAddPieceSync,
                 outcome::result<PieceInfo>(
                     const SectorRef &sector,
                     gsl::span<const UnpaddedPieceSize> piece_sizes,
                     const UnpaddedPieceSize &new_piece_size,
                     int,
                     uint64_t priority));

    MOCK_METHOD4(doAddNullPieceSync,
                 outcome::result<PieceInfo>(
                     const SectorRef &sector,
                     gsl::span<const UnpaddedPieceSize> piece_sizes,
                     const UnpaddedPieceSize &new_piece_size,
                     uint64_t priority));

    outcome::result<PieceInfo> addPieceSync(
        const SectorRef &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        proofs::PieceData piece_data,
        uint64_t priority) override {
      if (piece_data.isNullData()) {
        return doAddNullPieceSync(
            sector, piece_sizes, new_piece_size, priority);
      }
      return doAddPieceSync(
          sector, piece_sizes, new_piece_size, piece_data.getFd(), priority);
    }

    MOCK_METHOD3(generateWinningPoSt,
                 outcome::result<std::vector<PoStProof>>(
                     ActorId miner_id,
                     gsl::span<const ExtendedSectorInfo> sector_info,
                     PoStRandomness randomness));

    MOCK_METHOD3(generateWindowPoSt,
                 outcome::result<WindowPoStResponse>(
                     ActorId miner_id,
                     gsl::span<const ExtendedSectorInfo> sector_info,
                     PoStRandomness randomness));
  };
}  // namespace fc::sector_storage
