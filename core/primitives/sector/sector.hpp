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

  /// This ordering, defines mappings to UInt in a way which MUST never change.
  enum class RegisteredProof : int64_t {
    StackedDRG32GiBSeal = 1,
    StackedDRG32GiBPoSt = 2,
    StackedDRG2KiBSeal = 3,
    StackedDRG2KiBPoSt = 4,
    StackedDRG8MiBSeal = 5,
    StackedDRG8MiBPoSt = 6,
    StackedDRG512MiBSeal = 7,
    StackedDRG512MiBPoSt = 8,
  };

  outcome::result<RegisteredProof> getRegisteredPoStProof(
      RegisteredProof proof);
  outcome::result<RegisteredProof> getRegisteredSealProof(
      RegisteredProof proof);

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
    int64_t challenge_index;
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

  struct PoStVerifyInfo {
    PoStRandomness randomness;
    /// From OnChainPoStVerifyInfo
    std::vector<PoStCandidate> candidates;
    std::vector<PoStProof> proofs;
    std::vector<SectorInfo> eligible_sectors;
    /// used to derive 32-byte prover ID
    ActorId prover;
    uint64_t challenge_count;
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
    InvalidPoStProof = 1,
    InvalidSealProof,
  };
}  // namespace fc::primitives::sector

OUTCOME_HPP_DECLARE_ERROR(fc::primitives::sector, Errors);

#endif  // CPP_FILECOIN_CORE_PROOFS_SECTOR_HPP
