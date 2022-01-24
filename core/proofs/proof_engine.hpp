/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/bitsutil.hpp"
#include "common/outcome.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/piece/piece_data.hpp"
#include "primitives/sector/sector.hpp"

#include <boost/filesystem.hpp>
#include <gsl/span>

namespace fc::proofs {
  namespace fs = boost::filesystem;

  using primitives::ActorId;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::piece::PaddedPieceSize;
  using primitives::piece::PieceData;
  using primitives::piece::PieceInfo;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::AggregateSealVerifyProofAndInfos;
  using primitives::sector::PoStProof;
  using primitives::sector::PoStRandomness;
  using primitives::sector::Proof;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::RegisteredUpdateProof;
  using primitives::sector::ReplicaUpdateInfo;
  using primitives::sector::SealRandomness;
  using primitives::sector::SealVerifyInfo;
  using primitives::sector::SectorInfo;
  using primitives::sector::Ticket;
  using primitives::sector::WindowPoStVerifyInfo;
  using primitives::sector::WinningPoStVerifyInfo;
  using Devices = std::vector<std::string>;
  using Phase1Output = std::vector<uint8_t>;
  using ChallengeIndexes = std::vector<uint64_t>;
  using UnsealedCID = CID;
  using Seed = primitives::sector::InteractiveRandomness;

  struct PrivateSectorInfo {
    SectorInfo info;
    std::string cache_dir_path;
    RegisteredPoStProof post_proof_type;
    std::string sealed_sector_path;
  };
  inline bool operator==(const PrivateSectorInfo &lhs,
                         const PrivateSectorInfo &rhs) {
    return lhs.info == rhs.info && lhs.cache_dir_path == rhs.cache_dir_path
           && lhs.post_proof_type == rhs.post_proof_type
           && lhs.sealed_sector_path == rhs.sealed_sector_path;
  }

  // SortedPrivateSectorInfo is a sorted vector of PrivateSectorInfo
  struct SortedPrivateSectorInfo {
   public:
    std::vector<PrivateSectorInfo> values;
  };

  inline bool operator==(const SortedPrivateSectorInfo &lhs,
                         const SortedPrivateSectorInfo &rhs) {
    return lhs.values == rhs.values;
  }

  struct RequiredPadding {
    std::vector<PaddedPieceSize> pads;
    PaddedPieceSize size;
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

  struct SealedAndUnsealedCID {
    CID sealed_cid;
    CID unsealed_cid;
  };

  inline bool operator==(const SealedAndUnsealedCID &lhs,
                         const SealedAndUnsealedCID &rhs) {
    return lhs.sealed_cid == rhs.sealed_cid
           && lhs.unsealed_cid == rhs.unsealed_cid;
  }

  inline SortedPrivateSectorInfo newSortedPrivateSectorInfo(
      gsl::span<const PrivateSectorInfo> replica_info) {
    SortedPrivateSectorInfo sorted_replica_info;
    sorted_replica_info.values.assign(replica_info.cbegin(),
                                      replica_info.cend());
    std::sort(sorted_replica_info.values.begin(),
              sorted_replica_info.values.end(),
              [](const PrivateSectorInfo &lhs, const PrivateSectorInfo &rhs) {
                return lhs.info.sealed_cid < rhs.info.sealed_cid;
              });

    return sorted_replica_info;
  }

  inline RequiredPadding getRequiredPadding(PaddedPieceSize old_length,
                                            PaddedPieceSize new_piece_length) {
    std::vector<PaddedPieceSize> pad_pieces;

    uint64_t to_fill((std::numeric_limits<uint64_t>::max() - old_length + 1)
                     % new_piece_length);

    PaddedPieceSize sum;
    auto n = common::countSetBits(to_fill);
    for (size_t i = 0; i < n; ++i) {
      auto next = common::countTrailingZeros(to_fill);
      auto piece_size = uint64_t(1) << next;
      to_fill ^= piece_size;

      pad_pieces.emplace_back(piece_size);
      sum += piece_size;
    }

    return {
        .pads = std::move(pad_pieces),
        .size = sum,
    };
  }

  inline UnpaddedPieceSize padPiece(const boost::filesystem::path &path) {
    auto size{fs::file_size(path)};
    auto unpadded{primitives::piece::paddedSize(size)};
    if (size != unpadded) {
      fs::resize_file(path, unpadded);
    }
    return unpadded;
  }

  class ProofEngine {
   public:
    virtual ~ProofEngine() = default;

    virtual outcome::result<WriteWithoutAlignmentResult> writeWithoutAlignment(
        RegisteredSealProof proof_type,
        const PieceData &piece_data,
        const UnpaddedPieceSize &piece_bytes,
        const std::string &staged_sector_file_path) = 0;

    virtual outcome::result<WriteWithAlignmentResult> writeWithAlignment(
        RegisteredSealProof proof_type,
        const PieceData &piece_data,
        const UnpaddedPieceSize &piece_bytes,
        const std::string &staged_sector_file_path,
        gsl::span<const UnpaddedPieceSize> existing_piece_sizes) = 0;

    virtual outcome::result<void> readPiece(
        PieceData output,
        const std::string &unsealed_file,
        const PaddedPieceSize &offset,
        const UnpaddedPieceSize &piece_size) = 0;

    /**
     * @brief  Seals the staged sector at staged_sector_path in place, saving
     * the resulting replica to sealed_sector_path
     */
    virtual outcome::result<Phase1Output> sealPreCommitPhase1(
        RegisteredSealProof proof_type,
        const std::string &cache_dir_path,
        const std::string &staged_sector_path,
        const std::string &sealed_sector_path,
        SectorNumber sector_num,
        ActorId miner_id,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces) = 0;

    virtual outcome::result<SealedAndUnsealedCID> sealPreCommitPhase2(
        gsl::span<const uint8_t> phase1_output,
        const std::string &cache_dir_path,
        const std::string &sealed_sector_path) = 0;

    virtual outcome::result<Phase1Output> sealCommitPhase1(
        RegisteredSealProof proof_type,
        const CID &sealed_cid,
        const CID &unsealed_cid,
        const std::string &cache_dir_path,
        const std::string &sealed_sector_path,
        SectorNumber sector_num,
        ActorId miner_id,
        const Ticket &ticket,
        const Seed &seed,
        gsl::span<const PieceInfo> pieces) = 0;

    virtual outcome::result<Proof> sealCommitPhase2(
        gsl::span<const uint8_t> phase1_output,
        SectorNumber sector_id,
        ActorId miner_id) = 0;

    /**
     * generatePieceCIDFromFile produces a piece CID for the provided data
     * stored in a given file(via path).
     */
    virtual outcome::result<CID> generatePieceCIDFromFile(
        RegisteredSealProof proof_type,
        const std::string &piece_file_path,
        UnpaddedPieceSize piece_size) = 0;

    /**
     * generatePieceCID produces a piece CID for the provided data
     * stored in a given pieceData.
     */
    virtual outcome::result<CID> generatePieceCID(
        RegisteredSealProof proof_type, gsl::span<const uint8_t> data) = 0;

    /**
     * generatePieceCID produces a piece CID for the provided data
     * stored in a given pieceData.
     */
    virtual outcome::result<CID> generatePieceCID(
        RegisteredSealProof proof_type,
        const PieceData &piece,
        UnpaddedPieceSize piece_size) = 0;

    /** GenerateDataCommitment produces a commitment for the sector containing
     * the provided pieces.
     */
    virtual outcome::result<CID> generateUnsealedCID(
        RegisteredSealProof proof_type,
        gsl::span<const PieceInfo> pieces,
        bool pad) = 0;

    virtual outcome::result<ChallengeIndexes>
    generateWinningPoStSectorChallenge(RegisteredPoStProof proof_type,
                                       ActorId miner_id,
                                       const PoStRandomness &randomness,
                                       uint64_t eligible_sectors_len) = 0;

    virtual outcome::result<std::vector<PoStProof>> generateWinningPoSt(
        ActorId miner_id,
        const SortedPrivateSectorInfo &private_replica_info,
        const PoStRandomness &randomness) = 0;

    virtual outcome::result<std::vector<PoStProof>> generateWindowPoSt(
        ActorId miner_id,
        const SortedPrivateSectorInfo &private_replica_info,
        const PoStRandomness &randomness) = 0;

    virtual outcome::result<bool> verifyWinningPoSt(
        const WinningPoStVerifyInfo &info) = 0;

    virtual outcome::result<bool> verifyWindowPoSt(
        const WindowPoStVerifyInfo &info) = 0;

    /**
     * VerifySeal returns true if the sealing operation from which its inputs
     * were derived was valid, and false if not.
     */
    virtual outcome::result<bool> verifySeal(const SealVerifyInfo &info) = 0;

    virtual outcome::result<void> aggregateSealProofs(
        AggregateSealVerifyProofAndInfos &aggregate,
        const std::vector<BytesIn> &proofs) = 0;

    virtual outcome::result<bool> verifyAggregateSeals(
        const AggregateSealVerifyProofAndInfos &aggregate) = 0;

    /**
     * Generates update proof for empty sector
     */
    virtual outcome::result<Bytes> generateUpdateProof(
        RegisteredUpdateProof proof_type,
        const CID &old_sealed_cid,
        const CID &new_sealed_cid,
        const CID &unsealed_cid,
        const std::string &new_replica_path,
        const std::string &new_replica_cache_path,
        const std::string &sector_key_path,
        const std::string &sector_key_cache_path) = 0;

    /**
     * Verifies update proof for empty sector.
     */
    virtual outcome::result<bool> verifyUpdateProof(
        const ReplicaUpdateInfo &info) = 0;

    /**
     * Unseals sector
     */
    virtual outcome::result<void> unseal(RegisteredSealProof proof_type,
                                         const std::string &cache_dir_path,
                                         const std::string &sealed_sector_path,
                                         const std::string &unseal_output_path,
                                         SectorNumber sector_num,
                                         ActorId miner_id,
                                         const Ticket &ticket,
                                         const UnsealedCID &unsealed_cid) = 0;

    virtual outcome::result<void> unsealRange(RegisteredSealProof proof_type,
                                              const std::string &cache_dir_path,
                                              const PieceData &seal_fd,
                                              const PieceData &unseal_fd,
                                              SectorNumber sector_num,
                                              ActorId miner_id,
                                              const Ticket &ticket,
                                              const UnsealedCID &unsealed_cid,
                                              uint64_t offset,
                                              uint64_t length) = 0;

    /**
     * @brief Unseals the sector at @sealed_path and returns the bytes for a
     * piece whose first (unpadded) byte begins at @offset and ends at @offset
     * plus @length, inclusive
     */
    virtual outcome::result<void> unsealRange(
        RegisteredSealProof proof_type,
        const std::string &cache_dir_path,
        const std::string &sealed_sector_path,
        const std::string &unseal_output_path,
        SectorNumber sector_num,
        ActorId miner_id,
        const Ticket &ticket,
        const UnsealedCID &unsealed_cid,
        uint64_t offset,
        uint64_t length) = 0;

    virtual outcome::result<void> clearCache(
        SectorSize sector_size, const std::string &cache_dir_path) = 0;

    /**
     * @brief Returns the version of the provided PoSt proof
     */
    virtual outcome::result<std::string> getPoStVersion(
        RegisteredPoStProof proof_type) = 0;

    /**
     * @brief  Returns the version of the provided seal proof type
     */
    virtual outcome::result<std::string> getSealVersion(
        RegisteredSealProof proof_type) = 0;

    /**
     * @brief Returns an array of strings containing the device names that can
     * be used
     */
    virtual outcome::result<Devices> getGPUDevices() = 0;
  };
}  // namespace fc::proofs
