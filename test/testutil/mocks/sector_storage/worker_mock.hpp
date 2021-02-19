/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "sector_storage/worker.hpp"

namespace fc::sector_storage {
  class WorkerMock : public Worker {
   public:
    MOCK_METHOD1(moveStorage, outcome::result<void>(const SectorId &));

    MOCK_METHOD4(fetch,
                 outcome::result<void>(const SectorId &,
                                       const SectorFileType &,
                                       PathType,
                                       AcquireMode));

    MOCK_METHOD5(unsealPiece,
                 outcome::result<void>(const SectorId &,
                                       UnpaddedByteIndex,
                                       const UnpaddedPieceSize &,
                                       const SealRandomness &,
                                       const CID &));

    outcome::result<bool> readPiece(PieceData output,
                                    const SectorId &sector,
                                    UnpaddedByteIndex offset,
                                    const UnpaddedPieceSize &size) override {
      return doReadPiece(output.getFd(), sector, offset, size);
    }

    MOCK_METHOD4(doReadPiece,
                 outcome::result<bool>(int,
                                       const SectorId &,
                                       UnpaddedByteIndex,
                                       const UnpaddedPieceSize &));

    MOCK_METHOD0(getInfo, outcome::result<primitives::WorkerInfo>());

    MOCK_METHOD0(getSupportedTask,
                 outcome::result<std::set<primitives::TaskType>>());
    MOCK_METHOD0(getAccessiblePaths,
                 outcome::result<std::vector<primitives::StoragePath>>());

    MOCK_METHOD3(sealPreCommit1,
                 outcome::result<PreCommit1Output>(const SectorId &,
                                                   const SealRandomness &,
                                                   gsl::span<const PieceInfo>));

    MOCK_METHOD2(sealPreCommit2,
                 outcome::result<SectorCids>(const SectorId &,
                                             const PreCommit1Output &));

    MOCK_METHOD5(sealCommit1,
                 outcome::result<Commit1Output>(const SectorId &,
                                                const SealRandomness &,
                                                const InteractiveRandomness &,
                                                gsl::span<const PieceInfo>,
                                                const SectorCids &));

    MOCK_METHOD2(sealCommit2,
                 outcome::result<Proof>(const SectorId &,
                                        const Commit1Output &));

    MOCK_METHOD2(finalizeSector,
                 outcome::result<void>(const SectorId &,
                                       const gsl::span<const Range> &));

    MOCK_METHOD1(remove, outcome::result<void>(const SectorId &));

    MOCK_METHOD4(addPiece,
                 outcome::result<PieceInfo>(const SectorId &,
                                            gsl::span<const UnpaddedPieceSize>,
                                            const UnpaddedPieceSize &,
                                            const proofs::PieceData &));
  };
}  // namespace fc::sector_storage
