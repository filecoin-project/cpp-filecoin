/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <memory>
#include <mutex>
#include "miner/storage_fsm/types.hpp"

namespace fc::mining {
  using api::SectorPreCommitInfo;
  using fc::mining::types::SectorInfo;
  using primitives::TokenAmount;
  using PrecommitCallback = std::function<void(outcome::result<CID>)>;

  class PreCommitBatcher {
   public:
    virtual outcome::result<void> addPreCommit(
        const SectorInfo &secInf,
        const TokenAmount &deposit,
        const SectorPreCommitInfo &pcPrecommitResultInfo,
        const PrecommitCallback &callback) = 0;
    virtual void forceSend() = 0;
    virtual ~PreCommitBatcher() = default;
  };

}  // namespace fc::mining
