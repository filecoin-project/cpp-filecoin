/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_MANAGER_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_MANAGER_MOCK_HPP

#include <gmock/gmock.h>

#include "sector_storage/manager.hpp"

namespace fc::sector_storage {
  class ManagerMock : public Manager {
   public:
    MOCK_METHOD2(serveHTTP,
                 void(const http::request<http::dynamic_body> &request,
                      http::response<http::dynamic_body> &response));

    MOCK_METHOD2(checkProvable,
                 outcome::result<std::vector<SectorId>>(
                     RegisteredProof, gsl::span<const SectorId>));

    MOCK_METHOD0(getSectorSize, SectorSize());

    outcome::result<void> readPiece(PieceData output,
                                    const SectorId &sector,
                                    UnpaddedByteIndex offset,
                                    const UnpaddedPieceSize &size,
                                    const SealRandomness &randomness,
                                    const CID &cid) override {
      return doReadPiece(output.getFd(), sector, offset, size, randomness, cid);
    }

    MOCK_METHOD6(doReadPiece,
                 outcome::result<void>(int fd,
                                       const SectorId &,
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

    MOCK_METHOD4(
        sealPreCommit1,
        outcome::result<PreCommit1Output>(const SectorId &sector,
                                          const SealRandomness &ticket,
                                          gsl::span<const PieceInfo> pieces,
                                          uint64_t priority));

    MOCK_METHOD3(
        sealPreCommit2,
        outcome::result<SectorCids>(const SectorId &sector,
                                    const PreCommit1Output &pre_commit_1_output,
                                    uint64_t priority));

    MOCK_METHOD6(
        sealCommit1,
        outcome::result<Commit1Output>(const SectorId &sector,
                                       const SealRandomness &ticket,
                                       const InteractiveRandomness &seed,
                                       gsl::span<const PieceInfo> pieces,
                                       const SectorCids &cids,
                                       uint64_t priority));

    MOCK_METHOD3(sealCommit2,
                 outcome::result<Proof>(const SectorId &sector,
                                        const Commit1Output &commit_1_output,
                                        uint64_t priority));

    MOCK_METHOD3(
        finalizeSector,
        outcome::result<void>(const SectorId &sector,
                              const gsl::span<const Range> &keep_unsealed,
                              uint64_t priority));

    MOCK_METHOD1(remove, outcome::result<void>(const SectorId &sector));

    MOCK_METHOD5(addPiece,
                 outcome::result<PieceInfo>(
                     const SectorId &sector,
                     gsl::span<const UnpaddedPieceSize> piece_sizes,
                     const UnpaddedPieceSize &new_piece_size,
                     const proofs::PieceData &piece_data,
                     uint64_t priority));

    MOCK_METHOD3(generateWinningPoSt,
                 outcome::result<std::vector<PoStProof>>(
                     ActorId miner_id,
                     gsl::span<const SectorInfo> sector_info,
                     PoStRandomness randomness));

    MOCK_METHOD3(generateWindowPoSt,
                 outcome::result<WindowPoStResponse>(
                     ActorId miner_id,
                     gsl::span<const SectorInfo> sector_info,
                     PoStRandomness randomness));
  };
}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_MANAGER_MOCK_HPP
