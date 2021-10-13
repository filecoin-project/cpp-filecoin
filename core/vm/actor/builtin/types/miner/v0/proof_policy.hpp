/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/proof_policy.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;
  using version::NetworkVersion;

  class ProofPolicy : public types::miner::ProofPolicy {
   public:
    outcome::result<ChainEpoch> getSealProofSectorMaximumLifetime(
        RegisteredSealProof proof, NetworkVersion nv) const override;

    outcome::result<StoragePower> getPoStProofConsensusMinerMinPower(
        RegisteredPoStProof proof) const override;

    outcome::result<uint64_t> getPoStProofWindowPoStPartitionSectors(
        RegisteredPoStProof proof) const override;
  };
  CBOR_NON(ProofPolicy);

}  // namespace fc::vm::actor::builtin::v0::miner
