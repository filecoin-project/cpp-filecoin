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

  using Proof = std::vector<uint8_t>;
  using Prover = Blob<32>;
  using Seed = Blob<32>;
  using Devices = std::vector<std::string>;
  using Phase1Output = std::vector<uint8_t>;
  using fc::primitives::sector::RegisteredProof;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::piece::PieceInfo;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::PoStCandidate;
  using primitives::sector::PoStRandomness;
  using primitives::sector::SealRandomness;
  using primitives::sector::SectorInfo;
  using primitives::sector::Ticket;

  // RawSealPreCommitOutput is used to acquire a seed from the chain for the
  // second step of Interactive PoRep.
  /*class RawSealPreCommitOutput {
   public:
    Comm comm_d;
    Comm comm_r;
  };*/

  struct PublicSectorInfo {
    RegisteredProof post_proof_type;
    CID sealed_cid;
    SectorNumber sector_num;
  };

  // SortedPublicSectorInfo is a sorted vector of PublicSectorInfo
  struct SortedPublicSectorInfo {
    std::vector<PublicSectorInfo> values;
  };

  struct PrivateSectorInfo : SectorInfo {
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

    static outcome::result<fc::proofs::WriteWithoutAlignmentResult>
    writeWithoutAlignment(RegisteredProof proof_type,
                          const std::string &piece_file_path,
                          const UnpaddedPieceSize &piece_bytes,
                          const std::string &staged_sector_file_path);

    // existing_piece_sizes should be UnpaddedPieceSize, but for prevent copy
    // array it is uint64 span
    static outcome::result<fc::proofs::WriteWithAlignmentResult>
    writeWithAlignment(RegisteredProof proof_type,
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
        const Prover &prover_id,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces);

    static outcome::result<std::pair<CID, CID>> sealPreCommitPhase2(
        gsl::span<const uint8_t> phase1_output,
        const std::string &cache_dir_path,
        const std::string &sealed_sector_path);

    /*static outcome::result<Proof> sealCommit(
        uint64_t sector_size,
        uint8_t porep_proof_partitions,
        const std::string &cache_dir_path,
        uint64_t sector_id,
        const Prover &prover_id,
        const Ticket &ticket,
        const Seed &seed,
        gsl::span<const PublicPieceInfo> pieces,
        const RawSealPreCommitOutput &rspco);*/

    /**
     * Unseals sector
     */
    /*static outcome::result<void> unseal(uint64_t sector_size,
                                        uint8_t porep_proof_partitions,
                                        const std::string &cache_dir_path,
                                        const std::string &sealed_sector_path,
                                        const std::string &unseal_output_path,
                                        uint64_t sector_id,
                                        const Prover &prover_id,
                                        const Ticket &ticket,
                                        const Comm &comm_d);*/

    /**
     * @brief Unseals the sector at @sealed_path and returns the bytes for a
     * piece whose first (unpadded) byte begins at @offset and ends at @offset
     * plus @num_bytes, inclusive
     */
    /*static outcome::result<void> unsealRange(
        uint64_t sector_size,
        uint8_t porep_proof_partitions,
        const std::string &cache_dir_path,
        const std::string &sealed_sector_path,
        const std::string &unseal_output_path,
        uint64_t sector_id,
        const Prover &prover_id,
        const Ticket &ticket,
        const Comm &comm_d,
        uint64_t offset,
        uint64_t length);*/

    /**
     * @brief Computes a sectors's comm_d given its pieces
     */
    /*static outcome::result<Comm> generateDataCommitment(
        uint64_t sector_size, gsl::span<const PublicPieceInfo> pieces);*/

    static outcome::result<CID> generatePieceCIDFromFile(
        RegisteredProof proof_type,
        const std::string &piece_file_path,
        UnpaddedPieceSize piece_size);

    static outcome::result<CID> generateUnsealedCID(
        RegisteredProof proof_type, gsl::span<PieceInfo> pieces);

    /**
     * @brief Generates a piece commitment for the provided byte source. Returns
     * an error if the byte source produced more than piece_size bytes
     */
    /*static outcome::result<Comm> generatePieceCommitmentFromFile(
        const std::string &piece_file_path, uint64_t piece_size);*/

    /**
     * @brief Generates proof-of-spacetime candidates for ElectionPoSt
     */
    static outcome::result<std::vector<PoStCandidateWithTicket>>
    generateCandidates(
        const Prover &prover_id,
        const PoStRandomness &randomness,
        uint64_t challenge_count,
        const SortedPrivateSectorInfo &sorted_private_replica_info);

    /**
     * @brief Generate a proof-of-spacetime
     */
    static outcome::result<Proof> generatePoSt(
        const Prover &prover_id,
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
        const Prover &prover_id);

    /**
     * @brief Verifies the output of some previous-run seal operation
     */
    /* static outcome::result<bool> verifySeal(uint64_t sector_size,
                                             const Comm &comm_r,
                                             const Comm &comm_d,
                                             const Prover &prover_id,
                                             const Ticket &ticket,
                                             const Seed &seed,
                                             uint64_t sector_id,
                                             gsl::span<const uint8_t> proof);*/

    /**
     * @brief Generates a ticket from a @partial_ticket
     */
    // static outcome::result<Ticket> finalizeTicket(const Ticket
    // &partial_ticket);

    /**
     * @brief Returns the number of user bytes that will fit into a staged
     * sector
     */
    //  static uint64_t getMaxUserBytesPerStagedSector(uint64_t sector_size);

    /**
     * @brief Produces a vector of strings, each representing the name of a
     * detected GPU device
     */
    //   static outcome::result<Devices> getGPUDevices();

   private:
    static fc::common::Logger logger_;
  };
}  // namespace fc::proofs

#endif  // CPP_FILECOIN_CORE_PROOFS_HPP
