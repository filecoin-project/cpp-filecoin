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
    MOCK_METHOD2(moveStorage,
                 outcome::result<CallId>(const SectorRef &, SectorFileType));

    MOCK_METHOD4(fetch,
                 outcome::result<CallId>(const SectorRef &,
                                         const SectorFileType &,
                                         PathType,
                                         AcquireMode));

    MOCK_METHOD5(unsealPiece,
                 outcome::result<CallId>(const SectorRef &,
                                         UnpaddedByteIndex,
                                         const UnpaddedPieceSize &,
                                         const SealRandomness &,
                                         const CID &));

    outcome::result<CallId> readPiece(PieceData output,
                                      const SectorRef &sector,
                                      UnpaddedByteIndex offset,
                                      const UnpaddedPieceSize &size) override {
      return doReadPiece(output.getFd(), sector, offset, size);
    }

    MOCK_METHOD4(doReadPiece,
                 outcome::result<CallId>(int,
                                         const SectorRef &,
                                         UnpaddedByteIndex,
                                         const UnpaddedPieceSize &));

    MOCK_METHOD0(getInfo, outcome::result<primitives::WorkerInfo>());

    MOCK_METHOD0(getSupportedTask,
                 outcome::result<std::set<primitives::TaskType>>());
    MOCK_METHOD0(getAccessiblePaths,
                 outcome::result<std::vector<primitives::StoragePath>>());

    MOCK_METHOD3(sealPreCommit1,
                 outcome::result<CallId>(const SectorRef &,
                                         const SealRandomness &,
                                         const std::vector<PieceInfo> &));

    MOCK_METHOD2(sealPreCommit2,
                 outcome::result<CallId>(const SectorRef &,
                                         const PreCommit1Output &));

    MOCK_METHOD5(sealCommit1,
                 outcome::result<CallId>(const SectorRef &,
                                         const SealRandomness &,
                                         const InteractiveRandomness &,
                                         const std::vector<PieceInfo> &,
                                         const SectorCids &));

    MOCK_METHOD2(sealCommit2,
                 outcome::result<CallId>(const SectorRef &,
                                         const Commit1Output &));

    MOCK_METHOD2(replicaUpdate,
                 outcome::result<CallId>(const SectorRef &,
                                         const std::vector<PieceInfo> &));

    MOCK_METHOD4(proveReplicaUpdate1,
                 outcome::result<CallId>(
                     const SectorRef &, const CID &, const CID &, const CID &));

    MOCK_METHOD5(proveReplicaUpdate2,
                 outcome::result<CallId>(const SectorRef &,
                                         const CID &,
                                         const CID &,
                                         const CID &,
                                         const Update1Output &));

    MOCK_METHOD2(finalizeSector,
                 outcome::result<CallId>(const SectorRef &,
                                         std::vector<Range>));
    MOCK_METHOD2(finalizeReplicaUpdate,
                 outcome::result<CallId>(const SectorRef &,
                                         std::vector<Range>));

    outcome::result<CallId> addPiece(const SectorRef &sector,
                                     VectorCoW<UnpaddedPieceSize> piece_sizes,
                                     const UnpaddedPieceSize &new_piece_size,
                                     PieceData piece_data) override {
      return doAddPiece(
          sector, piece_sizes.mut(), new_piece_size, piece_data.getFd());
    }

    MOCK_METHOD4(doAddPiece,
                 outcome::result<CallId>(const SectorRef &,
                                         const std::vector<UnpaddedPieceSize> &,
                                         const UnpaddedPieceSize &,
                                         int));

    MOCK_CONST_METHOD0(isLocalWorker, bool());
  };
}  // namespace fc::sector_storage
