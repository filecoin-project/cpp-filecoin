/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PROOFS_SECTOR_HPP
#define CPP_FILECOIN_CORE_PROOFS_SECTOR_HPP

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"

namespace fc::primitives::sector {
  using common::Buffer;
  using crypto::randomness::Randomness;
  using primitives::ActorId;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::SectorNumber;

  struct SectorId {
    ActorId miner;
    SectorNumber sector;
  };
  inline bool operator<(const SectorId &lhs, const SectorId &rhs) {
    return lhs.miner < rhs.miner && lhs.sector < rhs.sector;
  }
  inline bool operator==(const SectorId &lhs, const SectorId &rhs) {
    return lhs.miner == rhs.miner && lhs.sector == rhs.sector;
  }

  /// This ordering, defines mappings to UInt in a way which MUST never change.
  enum class RegisteredProof : int64_t {
    StackedDRG32GiBSeal = 1,
    StackedDRG32GiBPoSt = 2,  // No longer used
    StackedDRG2KiBSeal = 3,
    StackedDRG2KiBPoSt = 4,  // No longer used
    StackedDRG8MiBSeal = 5,
    StackedDRG8MiBPoSt = 6,  // No longer used
    StackedDRG512MiBSeal = 7,
    StackedDRG512MiBPoSt = 8,  // No longer used

    StackedDRG2KiBWinningPoSt = 9,
    StackedDRG2KiBWindowPoSt = 10,

    StackedDRG8MiBWinningPoSt = 11,
    StackedDRG8MiBWindowPoSt = 12,

    StackedDRG512MiBWinningPoSt = 13,
    StackedDRG512MiBWindowPoSt = 14,

    StackedDRG32GiBWinningPoSt = 15,
    StackedDRG32GiBWindowPoSt = 16,

    StackedDRG64GiBSeal = 17,
    StackedDRG64GiBWinningPoSt = 18,
    StackedDRG64GiBWindowPoSt = 19,
  };

  outcome::result<RegisteredProof> getRegisteredWindowPoStProof(
      RegisteredProof proof);
  outcome::result<RegisteredProof> getRegisteredWinningPoStProof(
      RegisteredProof proof);
  outcome::result<RegisteredProof> getRegisteredSealProof(
      RegisteredProof proof);

  outcome::result<SectorSize> getSectorSize(RegisteredProof proof);

  outcome::result<RegisteredProof> sealProofTypeFromSectorSize(SectorSize size);

  outcome::result<size_t> getWindowPoStPartitionSectors(RegisteredProof proof);

  using SealRandomness = Randomness;

  using Ticket = SealRandomness;

  using InteractiveRandomness = Randomness;

  using Proof = std::vector<uint8_t>;

  /**
   * OnChainSealVerifyInfo is the structure of information that must be sent
   * with a message to commit a sector. Most of this information is not needed
   * in the state tree but will be verified in sm.CommitSector. See
   * SealCommitment for data stored on the state tree for each sector.
   */
  struct OnChainSealVerifyInfo {
    /// CommR
    CID sealed_cid;
    /// Used to derive the interactive PoRep challenge.
    ChainEpoch interactive_epoch;
    RegisteredProof registered_proof;
    Proof proof;
    std::vector<DealId> deals;
    SectorNumber sector;
    /// Used to tie the seal to a chain.
    ChainEpoch seal_rand_epoch;
  };

  /**
   * SealVerifyInfo is the structure of all the information a verifier needs to
   * verify a Seal.
   */
  struct SealVerifyInfo {
    SectorId sector;
    OnChainSealVerifyInfo info;
    SealRandomness randomness;
    InteractiveRandomness interactive_randomness;
    /// CommD
    CID unsealed_cid;
  };

  using PoStRandomness = Randomness;

  struct PoStProof {
    RegisteredProof registered_proof;
    Proof proof;
  };

  inline bool operator==(const PoStProof &lhs, const PoStProof &rhs) {
    return lhs.registered_proof == rhs.registered_proof
           && lhs.proof == rhs.proof;
  }

  struct PrivatePoStCandidateProof {
    RegisteredProof registered_proof;
    Buffer externalized;
  };

  struct PoStCandidate {
    RegisteredProof registered_proof;
    /**
     * Optional — will eventually be omitted for SurprisePoSt verification,
     * needed for now.
     */
    Ticket partial_ticket;
    /// Optional — should be ommitted for verification.
    PrivatePoStCandidateProof private_proof;
    SectorId sector;
    uint64_t challenge_index;
  };

  struct OnChainPoStVerifyInfo {
    RegisteredProof proof_type;
    std::vector<PoStCandidate> candidates;
    std::vector<PoStProof> proofs;
  };

  struct SectorInfo {
    // RegisteredProof used when sealing - needs to be mapped to PoSt registered
    // proof when used to verify a PoSt
    RegisteredProof registered_proof;
    uint64_t sector;
    /// CommR
    CID sealed_cid;
  };

  // Information needed to verify a Winning PoSt attached to a block header.
  // Note: this is not used within the state machine, but by the
  // consensus/election mechanisms.
  struct WinningPoStVerifyInfo {
    PoStRandomness randomness;
    std::vector<PoStProof> proofs;
    std::vector<SectorInfo> challenged_sectors;
    ActorId prover;
  };

  // Information needed to verify a Window PoSt submitted directly to a miner
  // actor.
  struct WindowPoStVerifyInfo {
    PoStRandomness randomness;
    std::vector<PoStProof> proofs;
    std::vector<SectorInfo> challenged_sectors;
    ActorId prover;
  };

  CBOR_TUPLE(SectorId, miner, sector)

  CBOR_TUPLE(OnChainSealVerifyInfo,
             sealed_cid,
             interactive_epoch,
             registered_proof,
             proof,
             deals,
             sector,
             seal_rand_epoch)

  CBOR_TUPLE(PoStProof, registered_proof, proof)

  CBOR_TUPLE(PrivatePoStCandidateProof, registered_proof, externalized)

  CBOR_TUPLE(PoStCandidate,
             registered_proof,
             partial_ticket,
             private_proof,
             sector,
             challenge_index)

  CBOR_TUPLE(OnChainPoStVerifyInfo, proof_type, candidates, proofs)

  enum class Errors {
    kInvalidPoStProof = 1,
    kInvalidSealProof,
    kInvalidProofType,
  };
}  // namespace fc::primitives::sector

OUTCOME_HPP_DECLARE_ERROR(fc::primitives::sector, Errors);

#endif  // CPP_FILECOIN_CORE_PROOFS_SECTOR_HPP
