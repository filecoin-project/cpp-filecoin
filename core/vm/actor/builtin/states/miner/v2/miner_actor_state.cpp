/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/miner/v2/miner_actor_state.hpp"

#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::sector::getRegisteredWindowPoStProof;
  using namespace types::miner;

  outcome::result<Universal<MinerInfo>> MinerActorState::getInfo() const {
    OUTCOME_TRY(info, miner_info.get());
    OUTCOME_TRYA(info->window_post_proof_type,
                 getRegisteredWindowPoStProof(info->seal_proof_type));
    return std::move(info);
  }

  outcome::result<std::vector<SectorOnChainInfo>>
  MinerActorState::rescheduleSectorExpirations(
      ChainEpoch curr_epoch,
      SectorSize ssize,
      const DeadlineSectorMap &deadline_sectors) {
    // todo
    return ERROR_TEXT("");
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

  outcome::result<TokenAmount> MinerActorState::unlockUnvestedFunds(
      ChainEpoch curr_epoch, const TokenAmount &target) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<TokenAmount> MinerActorState::unlockVestedFunds(
      ChainEpoch curr_epoch) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<TokenAmount> MinerActorState::getUnlockedBalance(
      const TokenAmount &actor_balance) const {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<TokenAmount> MinerActorState::getAvailableBalance(
      const TokenAmount &actor_balance) const {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<void> MinerActorState::checkBalanceInvariants(
      const TokenAmount &balance) const {
    // todo
    return ERROR_TEXT("");
  }

}  // namespace fc::vm::actor::builtin::v2::miner
