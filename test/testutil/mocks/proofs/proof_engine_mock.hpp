/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proofs/proof_engine.hpp"

#include <gmock/gmock.h>

namespace fc::proofs {

  class ProofEngineMock : public ProofEngine {
   public:
    MOCK_METHOD4(
        writeWithoutAlignment,
        outcome::result<WriteWithoutAlignmentResult>(RegisteredSealProof,
                                                     const PieceData &,
                                                     const UnpaddedPieceSize &,
                                                     const std::string &));

    MOCK_METHOD5(writeWithAlignment,
                 outcome::result<WriteWithAlignmentResult>(
                     RegisteredSealProof proof_type,
                     const PieceData &piece_data,
                     const UnpaddedPieceSize &piece_bytes,
                     const std::string &staged_sector_file_path,
                     gsl::span<const UnpaddedPieceSize> existing_piece_sizes));

    outcome::result<void> readPiece(
        PieceData output,
        const std::string &unsealed_file,
        const PaddedPieceSize &offset,
        const UnpaddedPieceSize &piece_size) override {
      return doReadPiece(output.getFd(), unsealed_file, offset, piece_size);
    };

    MOCK_METHOD4(doReadPiece,
                 outcome::result<void>(int,
                                       const std::string &,
                                       const PaddedPieceSize &,
                                       const UnpaddedPieceSize &));

    MOCK_METHOD8(sealPreCommitPhase1,
                 outcome::result<Phase1Output>(RegisteredSealProof,
                                               const std::string &,
                                               const std::string &,
                                               const std::string &,
                                               SectorNumber,
                                               ActorId,
                                               const SealRandomness &,
                                               gsl::span<const PieceInfo>));

    MOCK_METHOD3(sealPreCommitPhase2,
                 outcome::result<SealedAndUnsealedCID>(gsl::span<const uint8_t>,
                                                       const std::string &,
                                                       const std::string &));

    MOCK_METHOD10(sealCommitPhase1,
                  outcome::result<Phase1Output>(RegisteredSealProof,
                                                const CID &,
                                                const CID &,
                                                const std::string &,
                                                const std::string &,
                                                SectorNumber,
                                                ActorId,
                                                const Ticket &,
                                                const Seed &,
                                                gsl::span<const PieceInfo>));

    MOCK_METHOD3(sealCommitPhase2,
                 outcome::result<Proof>(gsl::span<const uint8_t>,
                                        SectorNumber,
                                        ActorId));

    MOCK_METHOD3(generatePieceCIDFromFile,
                 outcome::result<CID>(RegisteredSealProof,
                                      const std::string &,
                                      UnpaddedPieceSize));

    MOCK_METHOD2(generatePieceCID,
                 outcome::result<CID>(RegisteredSealProof,
                                      gsl::span<const uint8_t>));

    MOCK_METHOD3(generatePieceCID,
                 outcome::result<CID>(RegisteredSealProof,
                                      const PieceData &,
                                      UnpaddedPieceSize));

    MOCK_METHOD3(generateUnsealedCID,
                 outcome::result<CID>(RegisteredSealProof,
                                      gsl::span<const PieceInfo>,
                                      bool));

    MOCK_METHOD4(
        generateWinningPoStSectorChallenge,
        outcome::result<ChallengeIndexes>(RegisteredPoStProof proof_type,
                                          ActorId miner_id,
                                          const PoStRandomness &randomness,
                                          uint64_t eligible_sectors_len));

    MOCK_METHOD3(generateWinningPoSt,
                 outcome::result<std::vector<PoStProof>>(
                     ActorId miner_id,
                     const SortedPrivateSectorInfo &private_replica_info,
                     const PoStRandomness &randomness));

    MOCK_METHOD3(
        generateWindowPoSt,
        outcome::result<std::vector<PoStProof>>(ActorId,
                                                const SortedPrivateSectorInfo &,
                                                const PoStRandomness &));

    MOCK_METHOD1(verifyWinningPoSt,
                 outcome::result<bool>(const WinningPoStVerifyInfo &));

    MOCK_METHOD1(verifyWindowPoSt,
                 outcome::result<bool>(const WindowPoStVerifyInfo &));

    MOCK_METHOD1(verifySeal, outcome::result<bool>(const SealVerifyInfo &));

    MOCK_METHOD2(aggregateSealProofs,
                 outcome::result<void>(AggregateSealVerifyProofAndInfos &,
                                       const std::vector<BytesIn> &));

    MOCK_METHOD1(
        verifyAggregateSeals,
        outcome::result<bool>(const AggregateSealVerifyProofAndInfos &));

    MOCK_METHOD8(generateUpdateProof,
                 outcome::result<Bytes>(RegisteredUpdateProof,
                                        const CID &,
                                        const CID &,
                                        const CID &,
                                        const std::string &,
                                        const std::string &,
                                        const std::string &,
                                        const std::string &));

    MOCK_METHOD1(verifyUpdateProof,
                 outcome::result<bool>(const ReplicaUpdateInfo &));

    MOCK_METHOD8(unseal,
                 outcome::result<void>(RegisteredSealProof proof_type,
                                       const std::string &,
                                       const std::string &,
                                       const std::string &,
                                       SectorNumber,
                                       ActorId,
                                       const Ticket &,
                                       const UnsealedCID &));

    outcome::result<void> unsealRange(RegisteredSealProof proof_type,
                                      const std::string &cache_dir_path,
                                      const PieceData &seal_fd,
                                      const PieceData &unseal_fd,
                                      SectorNumber sector_num,
                                      ActorId miner_id,
                                      const Ticket &ticket,
                                      const UnsealedCID &unsealed_cid,
                                      uint64_t offset,
                                      uint64_t length) override {
      return doUnsealRange(proof_type,
                           cache_dir_path,
                           seal_fd.getFd(),
                           unseal_fd.getFd(),
                           sector_num,
                           miner_id,
                           ticket,
                           unsealed_cid,
                           offset,
                           length);
    }

    MOCK_METHOD10(doUnsealRange,
                  outcome::result<void>(RegisteredSealProof,
                                        const std::string &,
                                        int,
                                        int,
                                        SectorNumber,
                                        ActorId,
                                        const Ticket &,
                                        const UnsealedCID &,
                                        uint64_t,
                                        uint64_t));

    MOCK_METHOD10(unsealRange,
                  outcome::result<void>(RegisteredSealProof,
                                        const std::string &,
                                        const std::string &,
                                        const std::string &,
                                        SectorNumber,
                                        ActorId,
                                        const Ticket &,
                                        const UnsealedCID &,
                                        uint64_t,
                                        uint64_t));

    MOCK_METHOD2(clearCache,
                 outcome::result<void>(SectorSize, const std::string &));

    MOCK_METHOD1(getPoStVersion,
                 outcome::result<std::string>(RegisteredPoStProof));

    MOCK_METHOD1(getSealVersion,
                 outcome::result<std::string>(RegisteredSealProof));

    MOCK_METHOD0(getGPUDevices, outcome::result<Devices>());
  };
}  // namespace fc::proofs
