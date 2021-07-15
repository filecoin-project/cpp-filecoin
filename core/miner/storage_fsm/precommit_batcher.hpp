/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <memory>
#include <mutex>
#include "miner/storage_fsm/types.hpp"

namespace fc::mining {
  using primitives::TokenAmount;
  using types::SectorInfo;
  using vm::actor::builtin::types::miner::SectorPreCommitInfo;
  using PrecommitCallback = std::function<void(const outcome::result<CID> &)>;

  class PreCommitBatcher {
   public:
    virtual outcome::result<void> addPreCommit(
        const SectorInfo &sector_info,
        const TokenAmount &deposit,
        const SectorPreCommitInfo &precommit_info,
        const PrecommitCallback &callback) = 0;
    virtual void forceSend() = 0;
    virtual ~PreCommitBatcher() = default;
  };

}  // namespace fc::mining
