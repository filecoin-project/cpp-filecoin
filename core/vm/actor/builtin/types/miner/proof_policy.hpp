/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/version/version.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::sector::getRegisteredWindowPoStProof;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;
  using version::NetworkVersion;

  class ProofPolicy {
   public:
    virtual ~ProofPolicy() = default;

    /**
     * @return the partition size, in sectors, associated with a seal proof
     * type. The partition size is the number of sectors proved in a single PoSt
     * proof.
     */
    inline outcome::result<uint64_t> getSealProofWindowPoStPartitionSectors(
        RegisteredSealProof proof) const {
      OUTCOME_TRY(post_proof, getRegisteredWindowPoStProof(proof));
      return getPoStProofWindowPoStPartitionSectors(post_proof);
    }

    /**
     * SectorMaximumLifetime is the maximum duration a sector sealed with this
     * proof may exist between activation and expiration.
     */
    virtual outcome::result<ChainEpoch> getSealProofSectorMaximumLifetime(
        RegisteredSealProof proof, NetworkVersion nv) const = 0;

    /**
     * The minimum power of an individual miner to meet the threshold for leader
     * election (in bytes). Motivation:
     * - Limits sybil generation
     * - Improves consensus fault detection
     * - Guarantees a minimum fee for consensus faults
     * - Ensures that a specific soundness for the power table
     * NOTE: We may be able to reduce this in the future, addressing consensus
     * faults with more complicated penalties, sybil generation with
     * crypto-economic mechanism, and PoSt soundness by increasing the
     * challenges for small miners.
     */
    virtual outcome::result<StoragePower> getPoStProofConsensusMinerMinPower(
        RegisteredPoStProof proof) const = 0;

    /**
     * @return the partition size, in sectors, associated with a Window PoSt
     * proof type. The partition size is the number of sectors proved in a
     * single PoSt proof.
     */
    virtual outcome::result<uint64_t> getPoStProofWindowPoStPartitionSectors(
        RegisteredPoStProof proof) const = 0;
  };
}  // namespace fc::vm::actor::builtin::types::miner
