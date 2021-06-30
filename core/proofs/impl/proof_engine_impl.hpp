/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proofs/proof_engine.hpp"

#include "common/logger.hpp"

namespace fc::proofs {
  class ProofEngineImpl : public ProofEngine {
   public:
    ProofEngineImpl();

    outcome::result<WriteWithoutAlignmentResult> writeWithoutAlignment(
        RegisteredSealProof proof_type,
        const PieceData &piece_data,
        const UnpaddedPieceSize &piece_bytes,
        const std::string &staged_sector_file_path) override;

    outcome::result<WriteWithAlignmentResult> writeWithAlignment(
        RegisteredSealProof proof_type,
        const PieceData &piece_data,
        const UnpaddedPieceSize &piece_bytes,
        const std::string &staged_sector_file_path,
        gsl::span<const UnpaddedPieceSize> existing_piece_sizes) override;

    outcome::result<void> readPiece(
        PieceData output,
        const std::string &unsealed_file,
        const PaddedPieceSize &offset,
        const UnpaddedPieceSize &piece_size) override;

    outcome::result<Phase1Output> sealPreCommitPhase1(
        RegisteredSealProof proof_type,
        const std::string &cache_dir_path,
        const std::string &staged_sector_path,
        const std::string &sealed_sector_path,
        SectorNumber sector_num,
        ActorId miner_id,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces) override;

    outcome::result<SealedAndUnsealedCID> sealPreCommitPhase2(
        gsl::span<const uint8_t> phase1_output,
        const std::string &cache_dir_path,
        const std::string &sealed_sector_path) override;

    outcome::result<Phase1Output> sealCommitPhase1(
        RegisteredSealProof proof_type,
        const CID &sealed_cid,
        const CID &unsealed_cid,
        const std::string &cache_dir_path,
        const std::string &sealed_sector_path,
        SectorNumber sector_num,
        ActorId miner_id,
        const Ticket &ticket,
        const Seed &seed,
        gsl::span<const PieceInfo> pieces) override;

    outcome::result<Proof> sealCommitPhase2(
        gsl::span<const uint8_t> phase1_output,
        SectorNumber sector_id,
        ActorId miner_id) override;

    outcome::result<CID> generatePieceCIDFromFile(
        RegisteredSealProof proof_type,
        const std::string &piece_file_path,
        UnpaddedPieceSize piece_size) override;

    outcome::result<CID> generatePieceCID(
        RegisteredSealProof proof_type, gsl::span<const uint8_t> data) override;

    outcome::result<CID> generatePieceCID(
        RegisteredSealProof proof_type,
        const PieceData &piece,
        UnpaddedPieceSize piece_size) override;

    outcome::result<CID> generateUnsealedCID(RegisteredSealProof proof_type,
                                             gsl::span<const PieceInfo> pieces,
                                             bool pad) override;

    outcome::result<ChallengeIndexes> generateWinningPoStSectorChallenge(
        RegisteredPoStProof proof_type,
        ActorId miner_id,
        const PoStRandomness &randomness,
        uint64_t eligible_sectors_len) override;

    outcome::result<std::vector<PoStProof>> generateWinningPoSt(
        ActorId miner_id,
        const SortedPrivateSectorInfo &private_replica_info,
        const PoStRandomness &randomness) override;

    outcome::result<std::vector<PoStProof>> generateWindowPoSt(
        ActorId miner_id,
        const SortedPrivateSectorInfo &private_replica_info,
        const PoStRandomness &randomness) override;

    outcome::result<bool> verifyWinningPoSt(
        const WinningPoStVerifyInfo &info) override;

    outcome::result<bool> verifyWindowPoSt(
        const WindowPoStVerifyInfo &info) override;

    outcome::result<bool> verifySeal(const SealVerifyInfo &info) override;

    outcome::result<bool> verifyAggregateSeals(
        const AggregateSealVerifyProofAndInfos &aggregate) override;

    outcome::result<void> unseal(RegisteredSealProof proof_type,
                                 const std::string &cache_dir_path,
                                 const std::string &sealed_sector_path,
                                 const std::string &unseal_output_path,
                                 SectorNumber sector_num,
                                 ActorId miner_id,
                                 const Ticket &ticket,
                                 const UnsealedCID &unsealed_cid) override;

    outcome::result<void> unsealRange(RegisteredSealProof proof_type,
                                      const std::string &cache_dir_path,
                                      const PieceData &seal_fd,
                                      const PieceData &unseal_fd,
                                      SectorNumber sector_num,
                                      ActorId miner_id,
                                      const Ticket &ticket,
                                      const UnsealedCID &unsealed_cid,
                                      uint64_t offset,
                                      uint64_t length) override;

    outcome::result<void> unsealRange(RegisteredSealProof proof_type,
                                      const std::string &cache_dir_path,
                                      const std::string &sealed_sector_path,
                                      const std::string &unseal_output_path,
                                      SectorNumber sector_num,
                                      ActorId miner_id,
                                      const Ticket &ticket,
                                      const UnsealedCID &unsealed_cid,
                                      uint64_t offset,
                                      uint64_t length) override;

    outcome::result<void> clearCache(
        SectorSize sector_size, const std::string &cache_dir_path) override;

    outcome::result<std::string> getPoStVersion(
        RegisteredPoStProof proof_type) override;

    outcome::result<std::string> getSealVersion(
        RegisteredSealProof proof_type) override;

    outcome::result<Devices> getGPUDevices() override;

   private:
    common::Logger logger_;
  };
}  // namespace fc::proofs
