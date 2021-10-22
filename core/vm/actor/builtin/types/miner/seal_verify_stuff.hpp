/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/cid/cid.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::SectorNumber;
  using primitives::sector::Proof;
  using primitives::sector::RegisteredSealProof;

  /**
   * SealVerifyParams is the structure of information that must be sent with a
   * message to commit a sector. Most of this information is not needed in the
   * state tree but will be verified in sm.CommitSector. See SealCommitment for
   * data stored on the state tree for each sector.
   */
  struct SealVerifyStuff {
    CID sealed_cid;
    ChainEpoch interactive_epoch{};
    RegisteredSealProof seal_proof{};
    Proof proof;
    std::vector<DealId> deal_ids;
    SectorNumber sector{};
    ChainEpoch seal_rand_epoch{};
  };

}  // namespace fc::vm::actor::builtin::types::miner
