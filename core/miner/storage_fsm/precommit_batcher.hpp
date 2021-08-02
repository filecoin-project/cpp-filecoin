/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/types.hpp"
#include "vm/actor/builtin/types/miner/miner_info.hpp"


namespace fc::mining {
  using primitives::TokenAmount;
  using types::SectorInfo;
  using primitives::address::Address;
  using vm::actor::builtin::types::miner::SectorPreCommitInfo;
  using vm::actor::builtin::types::miner::MinerInfo;
  using PrecommitCallback = std::function<outcome::result<void>(const outcome::result<CID> &)>;

  class PreCommitBatcher {
   public:
    virtual ~PreCommitBatcher() = default;

    virtual outcome::result<void> addPreCommit(
        const SectorInfo &sector_info,
        const TokenAmount &deposit,
        const SectorPreCommitInfo &precommit_info,
        const PrecommitCallback &callback) = 0;

    virtual void forceSend() = 0;
  };

}  // namespace fc::mining
