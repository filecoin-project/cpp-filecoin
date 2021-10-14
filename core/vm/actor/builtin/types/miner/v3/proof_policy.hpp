/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/v2/proof_policy.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;
  using version::NetworkVersion;

  class ProofPolicy : public v2::miner::ProofPolicy {
   public:
    outcome::result<ChainEpoch> getSealProofSectorMaximumLifetime(
        RegisteredSealProof proof, NetworkVersion nv) const override;

    outcome::result<StoragePower> getPoStProofConsensusMinerMinPower(
        RegisteredPoStProof proof) const override;
  };
  CBOR_NON(ProofPolicy);

}  // namespace fc::vm::actor::builtin::v3::miner
