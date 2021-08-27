/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/miner/v3/miner_actor_state.hpp"

#include "common/error_text.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using namespace types::miner;

  outcome::result<Universal<MinerInfo>> MinerActorState::getInfo() const {
    return miner_info.get();
  }

  outcome::result<PowerPair> MinerActorState::assignSectorsToDeadlines(
      Runtime &runtime,
      ChainEpoch curr_epoch,
      const std::vector<SectorOnChainInfo> &sectors_to_assign,
      uint64_t partition_size,
      SectorSize ssize) {
    // todo
    return ERROR_TEXT("");
  }

}  // namespace fc::vm::actor::builtin::v3::miner
