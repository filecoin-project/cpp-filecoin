/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/cmp.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"
#include "vm/version/version.hpp"

namespace fc::primitives::sector {
  using crypto::randomness::Randomness;
  using primitives::ActorId;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::SectorNumber;
  using vm::version::NetworkVersion;

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
    kUndefined = -1,

    kStackedDrg2KiBV1,
    kStackedDrg8MiBV1,
    kStackedDrg512MiBV1,
    kStackedDrg32GiBV1,
    kStackedDrg64GiBV1,

    kStackedDrg2KiBV1_1,
    kStackedDrg8MiBV1_1,
    kStackedDrg512MiBV1_1,
    kStackedDrg32GiBV1_1,
    kStackedDrg64GiBV1_1,
  };

  enum class RegisteredPoStProof : int64_t {
    kUndefined = -1,

    kStackedDRG2KiBWinningPoSt,
    kStackedDRG8MiBWinningPoSt,
    kStackedDRG512MiBWinningPoSt,
    kStackedDRG32GiBWinningPoSt,
    kStackedDRG64GiBWinningPoSt,

    kStackedDRG2KiBWindowPoSt,
    kStackedDRG8MiBWindowPoSt,
    kStackedDRG512MiBWindowPoSt,
    kStackedDRG32GiBWindowPoSt,
    kStackedDRG64GiBWindowPoSt,
  };

  enum class RegisteredAggregationProof : int64_t {
    SnarkPackV1,
  };

  /**
   * Empty sector update.
   */
  enum class RegisteredUpdateProof : int64_t {
    kUndefined = -1,

    kStackedDrg2KiBV1,
    kStackedDrg8MiBV1,
    kStackedDrg512MiBV1,
    kStackedDrg32GiBV1,
    kStackedDrg64GiBV1,
  };

  /**
   * Produces the PoSt-specific RegisteredProof corresponding to the receiving
   * RegisteredSealProof.
   */
  outcome::result<RegisteredPoStProof> getRegisteredWindowPoStProof(
      RegisteredSealProof proof);
  outcome::result<RegisteredPoStProof> getRegisteredWinningPoStProof(
      RegisteredSealProof proof);
  outcome::result<RegisteredPoStProof> getRegisteredWinningPoStProof(
      RegisteredPoStProof proof);
  outcome::result<RegisteredUpdateProof> getRegisteredUpdateProof(
      RegisteredSealProof proof);

  outcome::result<SectorSize> getSectorSize(RegisteredSealProof proof);
  outcome::result<SectorSize> getSectorSize(RegisteredPoStProof proof);

  outcome::result<size_t> getWindowPoStPartitionSectors(
      RegisteredPoStProof proof);

  struct SectorRef {
    SectorId id{};
    RegisteredSealProof proof_type{RegisteredSealProof::kUndefined};
  };

  inline bool operator==(const SectorRef &lhs, const SectorRef &rhs) {
    return lhs.id == rhs.id and lhs.proof_type == rhs.proof_type;
  }

  /**
   * Returns the partition size, in sectors, associated with a seal proof type.
   * The partition size is the number of sectors proved in a single PoSt proof.
   * @param proof
   * @return
   */
  outcome::result<size_t> getSealProofWindowPoStPartitionSectors(
      RegisteredSealProof proof);

  using SealRandomness = Randomness;

  using Ticket = SealRandomness;

  using InteractiveRandomness = Randomness;

  using Proof = std::vector<uint8_t>;

  /**
   * SealVerifyInfo is the structure of all the information a verifier needs
   * to verify a Seal.
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
    RegisteredPoStProof registered_proof = RegisteredPoStProof::kUndefined;
    Proof proof;
  };

  inline bool operator==(const PoStProof &lhs, const PoStProof &rhs) {
    return lhs.registered_proof == rhs.registered_proof
           && lhs.proof == rhs.proof;
  }

  struct SectorInfo {
    RegisteredSealProof registered_proof;
    SectorNumber sector;
    /// CommR
    CID sealed_cid;
  };
  CBOR_TUPLE(SectorInfo, registered_proof, sector, sealed_cid)

  inline bool operator==(const SectorInfo &lhs, const SectorInfo &rhs) {
    return lhs.registered_proof == rhs.registered_proof
           && lhs.sector == rhs.sector && lhs.sealed_cid == rhs.sealed_cid;
  }

  struct ExtendedSectorInfo {
    RegisteredSealProof registered_proof;
    SectorNumber sector;
    boost::optional<CID> sector_key;
    /// CommR
    CID sealed_cid;
  };

  /**
   * Converts to SectorInfo truncating sector_key
   * @param extended_sector_info of ExtendedSectorInfo type
   * @return sector_info
   */
  inline SectorInfo toSectorInfo(
      const ExtendedSectorInfo &extended_sector_info) {
    return SectorInfo{
        .registered_proof = extended_sector_info.registered_proof,
        .sector = extended_sector_info.sector,
        .sealed_cid = extended_sector_info.sealed_cid,
    };
  }

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

  /**
   * Returns SealProofType for WindowPoStType taking into account the network
   * version
   * @param network_version - network version
   * @param proof - RegisteredPoStProof type
   * @return appropriate RegisteredSealProof
   */
  outcome::result<RegisteredSealProof>
  getPreferredSealProofTypeFromWindowPoStType(NetworkVersion network_version,
                                              RegisteredPoStProof proof);

  struct AggregateSealVerifyInfo {
    SectorNumber number;
    SealRandomness randomness;
    InteractiveRandomness interactive_randomness;
    CID sealed_cid;
    CID unsealed_cid;
  };
  CBOR_TUPLE(AggregateSealVerifyInfo,
             number,
             randomness,
             interactive_randomness,
             sealed_cid,
             unsealed_cid)

  struct AggregateSealVerifyProofAndInfos {
    ActorId miner;
    RegisteredSealProof seal_proof;
    RegisteredAggregationProof aggregate_proof;
    Bytes proof;
    std::vector<AggregateSealVerifyInfo> infos;
  };
  CBOR_TUPLE(AggregateSealVerifyProofAndInfos,
             miner,
             seal_proof,
             aggregate_proof,
             proof,
             infos)

  struct ReplicaUpdateInfo {
    RegisteredUpdateProof update_proof_type{};
    CID old_sealed_sector_cid;
    CID new_sealed_sector_cid;
    CID new_unsealed_sector_cid;
    Bytes proof;
  };
  CBOR_TUPLE(ReplicaUpdateInfo,
             update_proof_type,
             old_sealed_sector_cid,
             new_sealed_sector_cid,
             new_unsealed_sector_cid,
             proof);
}  // namespace fc::primitives::sector

OUTCOME_HPP_DECLARE_ERROR(fc::primitives::sector, Errors);
