/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PROOFS_HPP
#define CPP_FILECOIN_CORE_PROOFS_HPP

#include <vector>
#include "common/blob.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::proofs {

  // kSingleProofPartitionProofLen denotes the number of bytes in a proof
  // generated with a single partition. The number of bytes in a proof increases
  // linearly with the number of partitions used when creating that proof.
  const int kSingleProofPartitionProofLen = 192;

  using common::Blob;
  using crypto::randomness::Randomness;

  using Seed = Blob<32>;
  using Devices = std::vector<std::string>;
  using Phase1Output = std::vector<uint8_t>;
  using fc::primitives::sector::RegisteredProof;
  using primitives::ActorId;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::piece::PieceInfo;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::InteractiveRandomness;
  using primitives::sector::PoStCandidate;
  using primitives::sector::PoStRandomness;
  using primitives::sector::Proof;
  using primitives::sector::SealRandomness;
  using primitives::sector::SealVerifyInfo;
  using primitives::sector::SectorInfo;
  using primitives::sector::Ticket;
  using SealedCID = CID;
  using UnsealedCID = CID;

  struct PublicSectorInfo {
    RegisteredProof post_proof_type;
    CID sealed_cid;
    SectorNumber sector_num;
  };

  // SortedPublicSectorInfo is a sorted vector of PublicSectorInfo
  struct SortedPublicSectorInfo {
    std::vector<PublicSectorInfo> values;
  };

  struct PrivateSectorInfo {
    SectorInfo info;
    std::string cache_dir_path;
    RegisteredProof post_proof_type;
    std::string sealed_sector_path;
  };

  // SortedPrivateSectorInfo is a sorted vector of PrivateSectorInfo
  struct SortedPrivateSectorInfo {
   public:
    std::vector<PrivateSectorInfo> values;
  };

  struct PoStCandidateWithTicket {
    PoStCandidate candidate;
    Ticket ticket;
  };

  struct WriteWithoutAlignmentResult {
    uint64_t total_write_unpadded = 0;
    CID piece_cid;
  };

  struct WriteWithAlignmentResult {
    uint64_t left_alignment_unpadded = 0;
    uint64_t total_write_unpadded = 0;
    CID piece_cid;
  };

  class Proofs {
   public:
    static fc::proofs::SortedPrivateSectorInfo newSortedPrivateSectorInfo(
        gsl::span<const PrivateSectorInfo> replica_info);

    static fc::proofs::SortedPublicSectorInfo newSortedPublicSectorInfo(
        gsl::span<const PublicSectorInfo> sector_info);

    static outcome::result<WriteWithoutAlignmentResult> writeWithoutAlignment(
        RegisteredProof proof_type,
        const std::string &piece_file_path,
        const UnpaddedPieceSize &piece_bytes,
        const std::string &staged_sector_file_path);

    // existing_piece_sizes should be UnpaddedPieceSize, but for prevent copy
    // array it is uint64 span
    static outcome::result<WriteWithAlignmentResult> writeWithAlignment(
        RegisteredProof proof_type,
        const std::string &piece_file_path,
        const UnpaddedPieceSize &piece_bytes,
        const std::string &staged_sector_file_path,
        gsl::span<const uint64_t> existing_piece_sizes);

    /**
     * @brief  Seals the staged sector at staged_sector_path in place, saving
     * the resulting replica to sealed_sector_path
     */
    static outcome::result<Phase1Output> sealPreCommitPhase1(
        RegisteredProof proof_type,
        const std::string &cache_dir_path,
        const std::string &staged_sector_path,
        const std::string &sealed_sector_path,
        SectorNumber sector_num,
        ActorId miner_id,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces);

    static outcome::result<std::pair<SealedCID, UnsealedCID>>
    sealPreCommitPhase2(gsl::span<const uint8_t> phase1_output,
                        const std::string &cache_dir_path,
                        const std::string &sealed_sector_path);

    static outcome::result<Phase1Output> sealCommitPhase1(
        RegisteredProof proof_type,
        const CID &sealed_cid,
        const CID &unsealed_cid,
        const std::string &cache_dir_path,
        SectorNumber sector_num,
        ActorId miner_id,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        gsl::span<const PieceInfo> pieces);

    static outcome::result<Proof> sealCommitPhase2(
        gsl::span<const uint8_t> phase1_output,
        SectorNumber sector_id,
        ActorId miner_id);

    static outcome::result<CID> generatePieceCIDFromFile(
        RegisteredProof proof_type,
        const std::string &piece_file_path,
        UnpaddedPieceSize piece_size);

    static outcome::result<CID> generateUnsealedCID(
        RegisteredProof proof_type, gsl::span<PieceInfo> pieces);

    /**
     * @brief Generates proof-of-spacetime candidates for ElectionPoSt
     */
    static outcome::result<std::vector<PoStCandidateWithTicket>>
    generateCandidates(
        ActorId miner_id,
        const PoStRandomness &randomness,
        uint64_t challenge_count,
        const SortedPrivateSectorInfo &sorted_private_replica_info);

    /**
     * @brief Generate a proof-of-spacetime
     */
    static outcome::result<Proof> generatePoSt(
        ActorId miner_id,
        const SortedPrivateSectorInfo &private_replica_info,
        const PoStRandomness &randomness,
        gsl::span<const PoStCandidate> winners);

    /**
     * @brief Verifies a proof-of-spacetime
     */
    static outcome::result<bool> verifyPoSt(
        const SortedPublicSectorInfo &public_sector_info,
        const PoStRandomness &randomness,
        uint64_t challenge_count,
        gsl::span<const uint8_t> proof,
        gsl::span<const PoStCandidate> winners,
        ActorId miner_id);

    /**
     * VerifySeal returns true if the sealing operation from which its inputs
     * were derived was valid, and false if not.
     */
    static outcome::result<bool> verifySeal(const SealVerifyInfo &info);

    /**
     * Unseals sector
     */
    static outcome::result<void> unseal(RegisteredProof proof_type,
                                        const std::string &cache_dir_path,
                                        const std::string &sealed_sector_path,
                                        const std::string &unseal_output_path,
                                        SectorNumber sector_num,
                                        ActorId miner_id,
                                        const Ticket &ticket,
                                        const UnsealedCID &unsealed_cid);

    /**
     * @brief Unseals the sector at @sealed_path and returns the bytes for a
     * piece whose first (unpadded) byte begins at @offset and ends at @offset
     * plus @num_bytes, inclusive
     */
    static outcome::result<void> unsealRange(
        RegisteredProof proof_type,
        const std::string &cache_dir_path,
        const std::string &sealed_sector_path,
        const std::string &unseal_output_path,
        SectorNumber sector_num,
        ActorId miner_id,
        const Ticket &ticket,
        const UnsealedCID &unsealed_cid,
        uint64_t offset,
        uint64_t length);

   private:
    static fc::common::Logger logger_;
  };
}  // namespace fc::proofs

#endif  // CPP_FILECOIN_CORE_PROOFS_HPP
