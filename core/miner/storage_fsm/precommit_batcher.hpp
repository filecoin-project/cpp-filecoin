/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "miner/storage_fsm/types.hpp"
#include "vm/actor/builtin/types/miner/miner_info.hpp"

namespace fc::mining {
  using api::FullNodeApi;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using types::SectorInfo;
  using vm::actor::builtin::types::miner::MinerInfo;
  using vm::actor::builtin::types::miner::SectorPreCommitInfo;
  using PrecommitCallback = std::function<void(const outcome::result<CID> &)>;
  using AddressSelector = std::function<outcome::result<Address>(
      const MinerInfo &miner_info,
      const TokenAmount &good_funds,
      const std::shared_ptr<FullNodeApi> &api)>;

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
