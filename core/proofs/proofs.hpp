/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PROOFS_HPP
#define CPP_FILECOIN_CORE_PROOFS_HPP

#include <common/logger.hpp>
#include <vector>
#include "common/blob.hpp"
#include "common/outcome.hpp"
#include "crypto/randomness/randomness_types.hpp"

namespace fc::proofs {

  // kCommitmentBytesLen is the number of bytes in a CommR, CommD, CommP, and
  // CommRStar.
  const int kCommitmentBytesLen = 32;

  using fc::common::Blob;
  using fc::crypto::randomness::Randomness;
  using Comm = Blob<kCommitmentBytesLen>;
  using Proof = std::vector<uint8_t>;
  using Prover = Blob<32>;
  using Ticket = Blob<32>;

  // RawSealPreCommitOutput is used to acquire a seed from the chain for the
  // second step of Interactive PoRep.
  class RawSealPreCommitOutput {
   public:
    Comm comm_d;
    Comm comm_r;
  };

  // PublicPieceInfo is an on-chain tuple of CommP and aligned piece-size.
  class PublicPieceInfo {
   public:
    uint64_t size;
    Comm comm_p;
  };

  class PublicSectorInfo {
   public:
    uint64_t sector_id;
    Comm comm_r;
  };

  // SortedPublicSectorInfo is a sorted vector of PublicSectorInfo
  class SortedPublicSectorInfo {
   public:
    std::vector<PublicSectorInfo> values;
  };

  class PrivateReplicaInfo {
   public:
    uint64_t sector_id;
    Comm comm_r;
    std::string cache_dir_path;
    std::string sealed_sector_path;
  };

  // SortedPrivateReplicaInfo is a sorted vector of PrivateReplicaInfo
  class SortedPrivateReplicaInfo {
   public:
    std::vector<PrivateReplicaInfo> values;
  };

  class Candidate {
   public:
    uint64_t sector_id;
    Ticket partial_ticket;
    Ticket ticket;
    uint64_t sector_challenge_index;
  };

  class WriteWithoutAlignmentResult {
   public:
    uint64_t total_write_unpadded;
    Comm comm_p;
  };

  class WriteWithAlignmentResult {
   public:
    uint64_t left_alignment_unpadded;
    uint64_t total_write_unpadded;
    Comm comm_p;
  };

  class Proofs {
   public:
    static fc::proofs::SortedPrivateReplicaInfo newSortedPrivateReplicaInfo(
        gsl::span<const PrivateReplicaInfo> replica_info);

    static fc::proofs::SortedPublicSectorInfo newSortedPublicSectorInfo(
        gsl::span<const PublicSectorInfo> sector_info);

    static outcome::result<fc::proofs::WriteWithoutAlignmentResult>
    writeWithoutAlignment(const std::string &piece_file_path,
                          uint64_t piece_bytes,
                          const std::string &staged_sector_file_path);

    static outcome::result<fc::proofs::WriteWithAlignmentResult>
    writeWithAlignment(const std::string &piece_file_path,
                       uint64_t piece_bytes,
                       const std::string &staged_sector_file_path,
                       gsl::span<const uint64_t> existing_piece_sizes);

    /**
     * @brief  Seals the staged sector at staged_sector_path in place, saving
     * the resulting replica to sealed_sector_path.
     */
    static outcome::result<RawSealPreCommitOutput> sealPreCommit(
        uint64_t sector_size,
        uint8_t porep_proof_partitions,
        const std::string &cache_dir_path,
        const std::string &staged_sector_path,
        const std::string &sealed_sector_path,
        uint64_t sector_id,
        const Prover &prover_id,
        const Ticket &ticket,
        gsl::span<const PublicPieceInfo> pieces);

    /**
     * @brief Computes a sectors's comm_d given its pieces.
     */
    static outcome::result<Comm> generateDataCommitment(
        uint64_t sector_size, gsl::span<const PublicPieceInfo> pieces);

    /**
     * @brief Generates a piece commitment for the provided byte source. Returns
     * an error if the byte source produced more than piece_size bytes.
     */
    static outcome::result<Comm> generatePieceCommitmentFromFile(
        const std::string &piece_file_path, uint64_t piece_size);

    /**
     * @brief Generates proof-of-spacetime candidates for ElectionPoSt
     */
    static outcome::result<std::vector<Candidate>> generateCandidates(
        uint64_t sector_size,
        const Prover &prover_id,
        const Randomness &randomness,
        uint64_t challenge_count,
        const SortedPrivateReplicaInfo &sorted_private_replica_info);

    /**
     * @brief Generate a proof-of-spacetime
     */
    static outcome::result<Proof> generatePoSt(
        uint64_t sectorSize,
        const Prover &prover_id,
        const SortedPrivateReplicaInfo &private_replica_info,
        const Randomness &randomness,
        gsl::span<const Candidate> winners);

    /**
     * @brief Verifies a proof-of-spacetime.
     */
    static outcome::result<bool> verifyPoSt(
        uint64_t sector_size,
        const SortedPublicSectorInfo &public_sector_info,
        const Randomness &randomness,
        uint64_t challenge_count,
        gsl::span<const uint8_t> proof,
        gsl::span<const Candidate> winners,
        const Prover &prover_id);

   private:
    static fc::common::Logger logger_;
  };
}  // namespace fc::proofs

#endif  // CPP_FILECOIN_CORE_PROOFS_HPP
