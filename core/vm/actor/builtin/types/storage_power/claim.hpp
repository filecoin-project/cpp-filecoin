/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::storage_power {
  using primitives::StoragePower;
  using primitives::sector::RegisteredSealProof;

  struct Claim {
    Claim() = default;
    Claim(const StoragePower &raw, const StoragePower &qa)
        : seal_proof_type(RegisteredSealProof::undefined),
          raw_power(raw),
          qa_power(qa) {}
    Claim(const StoragePower &raw,
          const StoragePower &qa,
          RegisteredSealProof seal_proof)
        : seal_proof_type(seal_proof), raw_power(raw), qa_power(qa) {}

    /** Miner's proof type used to determine minimum miner size */
    RegisteredSealProof seal_proof_type;

    /** Sum of raw byte power for a miner's sectors */
    StoragePower raw_power;

    /** Sum of quality adjusted power for a miner's sectors */
    StoragePower qa_power;

    inline bool operator==(const Claim &other) const {
      return seal_proof_type == other.seal_proof_type
             && raw_power == other.raw_power && qa_power == other.qa_power;
    }
  };

}  // namespace fc::vm::actor::builtin::types::storage_power
