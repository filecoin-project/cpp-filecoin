/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PROOFS_SECTOR_HPP
#define CPP_FILECOIN_CORE_PROOFS_SECTOR_HPP

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "common/cmp.hpp"
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
    return less(lhs.miner, rhs.miner, lhs.sector, rhs.sector);
  }
  inline bool operator==(const SectorId &lhs, const SectorId &rhs) {
    return lhs.miner == rhs.miner && lhs.sector == rhs.sector;
  }

  enum class RegisteredSealProof : int64_t {
    StackedDrg2KiBV1,
    StackedDrg8MiBV1,
    StackedDrg512MiBV1,
    StackedDrg32GiBV1,
    StackedDrg64GiBV1,

    StackedDrg2KiBV1_1,
    StackedDrg8MiBV1_1,
    StackedDrg512MiBV1_1,
    StackedDrg32GiBV1_1,
    StackedDrg64GiBV1_1,
  };

  enum class RegisteredPoStProof : int64_t {
    StackedDRG2KiBWinningPoSt,
    StackedDRG8MiBWinningPoSt,
    StackedDRG512MiBWinningPoSt,
    StackedDRG32GiBWinningPoSt,
    StackedDRG64GiBWinningPoSt,

    StackedDRG2KiBWindowPoSt,
    StackedDRG8MiBWindowPoSt,
    StackedDRG512MiBWindowPoSt,
    StackedDRG32GiBWindowPoSt,
    StackedDRG64GiBWindowPoSt,
  };

  outcome::result<RegisteredPoStProof> getRegisteredWindowPoStProof(
      RegisteredSealProof proof);
  outcome::result<RegisteredPoStProof> getRegisteredWinningPoStProof(
      RegisteredSealProof proof);

  outcome::result<SectorSize> getSectorSize(RegisteredSealProof proof);
  outcome::result<SectorSize> getSectorSize(RegisteredPoStProof proof);

  outcome::result<size_t> getWindowPoStPartitionSectors(
      RegisteredPoStProof proof);

  using SealRandomness = Randomness;

  using Ticket = SealRandomness;

  using InteractiveRandomness = Randomness;

  using Proof = std::vector<uint8_t>;

  /**
   * SealVerifyInfo is the structure of all the information a verifier needs to
   * verify a Seal.
   */
  struct SealVerifyInfo {
    RegisteredSealProof seal_proof;
    SectorId sector;
    std::vector<DealId> deals;
    SealRandomness randomness;
    InteractiveRandomness interactive_randomness;
    Proof proof;
    /// CommR
    CID sealed_cid;
    /// CommD
    CID unsealed_cid;
  };
  CBOR_TUPLE(SealVerifyInfo,
             seal_proof,
             sector,
             deals,
             randomness,
             interactive_randomness,
             proof,
             sealed_cid,
             unsealed_cid)

  using PoStRandomness = Randomness;

  struct PoStProof {
    RegisteredPoStProof registered_proof;
    Proof proof;
  };

  inline bool operator==(const PoStProof &lhs, const PoStProof &rhs) {
    return lhs.registered_proof == rhs.registered_proof
           && lhs.proof == rhs.proof;
  }

  struct SectorInfo {
    RegisteredSealProof registered_proof;
    uint64_t sector;
    /// CommR
    CID sealed_cid;
  };
  CBOR_TUPLE(SectorInfo, registered_proof, sector, sealed_cid)

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
  CBOR_TUPLE(
      WindowPoStVerifyInfo, randomness, proofs, challenged_sectors, prover)

  CBOR_TUPLE(SectorId, miner, sector)

  CBOR_TUPLE(PoStProof, registered_proof, proof)

  enum class Errors {
    kInvalidPoStProof = 1,
    kInvalidSealProof,
    kInvalidProofType,
  };
}  // namespace fc::primitives::sector

OUTCOME_HPP_DECLARE_ERROR(fc::primitives::sector, Errors);

#endif  // CPP_FILECOIN_CORE_PROOFS_SECTOR_HPP
