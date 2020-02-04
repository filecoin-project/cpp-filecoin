/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PROOFS_HPP
#define CPP_FILECOIN_CORE_PROOFS_HPP

#include <vector>
#include "common/blob.hpp"
#include "common/outcome.hpp"
#include "crypto/randomness/randomness_types.hpp"

using fc::common::Blob;

namespace fc::proofs {

  // kCommitmentBytesLen is the number of bytes in a CommR, CommD, CommP, and
  // CommRStar.
  const int kCommitmentBytesLen = 32;

  // RawSealPreCommitOutput is used to acquire a seed from the chain for the
  // second step of Interactive PoRep.
  class RawSealPreCommitOutput {
   public:
    fc::common::Blob<kCommitmentBytesLen> comm_d;
    fc::common::Blob<kCommitmentBytesLen> comm_r;
  };

  // PublicPieceInfo is an on-chain tuple of CommP and aligned piece-size.
  class PublicPieceInfo {
   public:
    uint64_t size;
    fc::common::Blob<kCommitmentBytesLen> comm_p;
  };

  class PublicSectorInfo {
   public:
    uint64_t sector_id;
    fc::common::Blob<kCommitmentBytesLen> comm_r;
  };

  // SortedPublicSectorInfo is a sorted vector of PublicSectorInfo
  class SortedPublicSectorInfo {
   public:
    std::vector<PublicSectorInfo> values;
  };

  class PrivateReplicaInfo {
   public:
    uint64_t sector_id;
    fc::common::Blob<kCommitmentBytesLen> comm_r;
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
    fc::common::Blob<32> partial_ticket;
    fc::common::Blob<32> ticket;
    uint64_t sector_challenge_index;
  };

  class WriteWithoutAlignmentResult {
   public:
    uint64_t total_write_unpadded;
    fc::common::Blob<kCommitmentBytesLen> comm_p;
  };

  class WriteWithAlignmentResult {
   public:
    uint64_t left_alignment_unpadded;
    uint64_t total_write_unpadded;
    fc::common::Blob<kCommitmentBytesLen> comm_p;
  };

  fc::proofs::SortedPrivateReplicaInfo newSortedPrivateReplicaInfo(
      gsl::span<const PrivateReplicaInfo> replica_info);

  fc::proofs::SortedPublicSectorInfo newSortedPublicSectorInfo(
      gsl::span<const PublicSectorInfo> sector_info);

  outcome::result<fc::proofs::WriteWithoutAlignmentResult>
  writeWithoutAlignment(const std::string &piece_file_path,
                        const uint64_t piece_bytes,
                        const std::string &staged_sector_file_path);

  outcome::result<fc::proofs::WriteWithAlignmentResult> writeWithAlignment(
      const std::string &piece_file_path,
      const uint64_t piece_bytes,
      const std::string &staged_sector_file_path,
      gsl::span<const uint64_t> existing_piece_sizes);

  /**
   * @brief  Seals the staged sector at staged_sector_path in place, saving the
   * resulting replica to sealed_sector_path.
   */
  outcome::result<RawSealPreCommitOutput> sealPreCommit(
      const uint64_t sector_size,
      const uint8_t porep_proof_partitions,
      const std::string &cache_dir_path,
      const std::string &staged_sector_path,
      const std::string &sealed_sector_path,
      const uint64_t sector_id,
      const fc::common::Blob<32> &prover_id,
      const fc::common::Blob<32> &ticket,
      gsl::span<const PublicPieceInfo> pieces);

  /**
   * @brief Computes a sectors's comm_d given its pieces.
   */
  outcome::result<fc::common::Blob<kCommitmentBytesLen>> generateDataCommitment(
      const uint64_t sector_size, gsl::span<const PublicPieceInfo> pieces);

  /**
   * @brief Generates a piece commitment for the provided byte source. Returns
   * an error if the byte source produced more than piece_size bytes.
   */
  outcome::result<fc::common::Blob<kCommitmentBytesLen>>
  generatePieceCommitmentFromFile(const std::string &piece_file_path,
                                  const uint64_t piece_size);

  /**
   * @brief Generates proof-of-spacetime candidates for ElectionPoSt
   */
  outcome::result<std::vector<Candidate>> generateCandidates(
      const uint64_t sector_size,
      const fc::common::Blob<32> &prover_id,
      const fc::crypto::randomness::Randomness &randomness,
      const uint64_t challenge_count,
      const SortedPrivateReplicaInfo &sorted_private_replica_info);

  /**
   * @brief Generate a proof-of-spacetime
   */
  outcome::result<std::vector<uint8_t>> generatePoSt(
      const uint64_t sectorSize,
      const fc::common::Blob<32> &prover_id,
      const SortedPrivateReplicaInfo &private_replica_info,
      const fc::crypto::randomness::Randomness &randomness,
      gsl::span<const Candidate> winners);

  /**
   * @brief Verifies a proof-of-spacetime.
   */
  outcome::result<bool> verifyPoSt(
      const uint64_t sector_size,
      const SortedPublicSectorInfo &public_sector_info,
      const fc::crypto::randomness::Randomness &randomness,
      const uint64_t challenge_count,
      gsl::span<const uint8_t> proof,
      gsl::span<const Candidate> winners,
      const fc::common::Blob<32> &prover_id);

}  // namespace fc::proofs

#endif  // CPP_FILECOIN_CORE_PROOFS_HPP
