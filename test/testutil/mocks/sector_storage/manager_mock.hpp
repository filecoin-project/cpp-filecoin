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

    outcome::result<void> readPiece(
        PieceData output,
        const SectorRef &sector,
        UnpaddedByteIndex offset,
        const UnpaddedPieceSize &size,
        const SealRandomness &randomness,
        const CID &cid,
        std::function<void(outcome::result<bool>)> cb) override {
      return doReadPiece(
          output.getFd(), sector, offset, size, randomness, cid, cb);
    }

    MOCK_METHOD7(
        doReadPiece,
        outcome::result<void>(int fd,
                              const SectorRef &,
                              UnpaddedByteIndex,
                              const UnpaddedPieceSize &,
                              const SealRandomness &,
                              const CID &,
                              std::function<void(outcome::result<bool>)>));

    outcome::result<bool> readPieceSync(PieceData output,
                                        const SectorRef &sector,
                                        UnpaddedByteIndex offset,
                                        const UnpaddedPieceSize &size,
                                        const SealRandomness &randomness,
                                        const CID &cid) override {
      return doReadPieceSync(
          output.getFd(), sector, offset, size, randomness, cid);
    }

    MOCK_METHOD6(doReadPieceSync,
                 outcome::result<bool>(int fd,
                                       const SectorRef &,
                                       UnpaddedByteIndex,
                                       const UnpaddedPieceSize &,
                                       const SealRandomness &,
                                       const CID &));

    MOCK_METHOD1(addLocalStorage,
                 outcome::result<void>(const std::string &path));

    MOCK_METHOD1(addWorker,
                 outcome::result<void>(std::shared_ptr<Worker> worker));

    MOCK_METHOD0(getLocalStorages,
                 outcome::result<std::unordered_map<StorageID, std::string>>());

    MOCK_METHOD1(getFsStat, outcome::result<FsStat>(StorageID storage_id));

    MOCK_METHOD5(sealPreCommit1,
                 outcome::result<void>(
                     const SectorRef &sector,
                     const SealRandomness &ticket,
                     const std::vector<PieceInfo> &pieces,
                     std::function<void(outcome::result<PreCommit1Output>)>,
                     uint64_t priority));
    MOCK_METHOD4(
        sealPreCommit1Sync,
        outcome::result<PreCommit1Output>(const SectorRef &sector,
                                          const SealRandomness &ticket,
                                          const std::vector<PieceInfo> &pieces,
                                          uint64_t priority));

    MOCK_METHOD3(
        sealPreCommit2Sync,
        outcome::result<SectorCids>(const SectorRef &sector,
                                    const PreCommit1Output &pre_commit_1_output,
                                    uint64_t priority));
    MOCK_METHOD4(sealPreCommit2,
                 outcome::result<void>(
                     const SectorRef &sector,
                     const PreCommit1Output &pre_commit_1_output,
                     std::function<void(outcome::result<SectorCids>)> cb,
                     uint64_t priority));

    MOCK_METHOD6(
        sealCommit1Sync,
        outcome::result<Commit1Output>(const SectorRef &sector,
                                       const SealRandomness &ticket,
                                       const InteractiveRandomness &seed,
                                       const std::vector<PieceInfo> &pieces,
                                       const SectorCids &cids,
                                       uint64_t priority));

    MOCK_METHOD7(sealCommit1,
                 outcome::result<void>(
                     const SectorRef &sector,
                     const SealRandomness &ticket,
                     const InteractiveRandomness &seed,
                     const std::vector<PieceInfo> &pieces,
                     const SectorCids &cids,
                     std::function<void(outcome::result<Commit1Output>)>,
                     uint64_t priority));

    MOCK_METHOD3(sealCommit2Sync,
                 outcome::result<Proof>(const SectorRef &sector,
                                        const Commit1Output &commit_1_output,
                                        uint64_t priority));
    MOCK_METHOD4(
        sealCommit2,
        outcome::result<void>(const SectorRef &sector,
                              const Commit1Output &commit_1_output,
                              std::function<void(outcome::result<Proof>)>,
                              uint64_t priority));

    MOCK_METHOD3(
        finalizeSectorSync,
        outcome::result<void>(const SectorRef &sector,
                              const gsl::span<const Range> &keep_unsealed,
                              uint64_t priority));

    MOCK_METHOD4(
        finalizeSector,
        outcome::result<void>(const SectorRef &sector,
                              const gsl::span<const Range> &keep_unsealed,
                              std::function<void(outcome::result<void>)>,
                              uint64_t priority));

    MOCK_METHOD1(remove, outcome::result<void>(const SectorRef &sector));

    MOCK_METHOD6(
        doAddPiece,
        outcome::result<void>(const SectorRef &sector,
                              gsl::span<const UnpaddedPieceSize> piece_sizes,
                              const UnpaddedPieceSize &new_piece_size,
                              int,
                              std::function<void(outcome::result<PieceInfo>)>,
                              uint64_t priority));

    outcome::result<void> addPiece(
        const SectorRef &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        proofs::PieceData piece_data,
        std::function<void(outcome::result<PieceInfo>)> cb,
        uint64_t priority) override {
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

    MOCK_METHOD4(doNullAddPieceSync,
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
        return doNullAddPieceSync(
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
