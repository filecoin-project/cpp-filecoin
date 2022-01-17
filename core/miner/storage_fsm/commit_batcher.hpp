/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "api/full_node/node_api.hpp"
#include "miner/storage_fsm/types.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::mining {
  using CommitCallback = std::function<void(const outcome::result<CID> &)>;
  using api::FullNodeApi;
  using primitives::sector::RegisteredSealProof;
  using sector_storage::Proof;
  using types::SectorInfo;

  struct AggregateInput {
    Proof proof;
    // Info info;
    RegisteredSealProof spt;
  };

  class CommitBatcher {
   public:
    virtual ~CommitBatcher() = default;

    virtual outcome::result<void> addCommit(
        const SectorInfo &sector_info,
        const AggregateInput &aggregate_input,
        const CommitCallback &callBack) = 0;

    virtual void forceSend() = 0;
  };

}  // namespace fc::mining
