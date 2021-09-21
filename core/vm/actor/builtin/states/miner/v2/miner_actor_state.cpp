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
      Runtime &runtime,
      ChainEpoch curr_epoch,
      SectorSize ssize,
      const DeadlineSectorMap &deadline_sectors) {
    OUTCOME_TRY(dls, this->deadlines.get());

    OUTCOME_TRY(sectors, this->sectors.loadSectors());

    std::vector<SectorOnChainInfo> all_replaced;

    for (const auto &[dl_id, pm] : deadline_sectors.map) {
      const auto dl_info =
          DeadlineInfo(this->proving_period_start, dl_id, curr_epoch)
              .nextNotElapsed();
      const auto new_expiration = dl_info.last();

      OUTCOME_TRY(deadline, dls.loadDeadline(dl_id));
      OUTCOME_TRY(
          replaced,
          deadline->rescheduleSectorExpirations(
              runtime, sectors, new_expiration, pm, ssize, dl_info.quant()));
      all_replaced.insert(all_replaced.end(), replaced.begin(), replaced.end());

      OUTCOME_TRY(dls.updateDeadline(dl_id, deadline));
    }

    OUTCOME_TRY(this->deadlines.set(dls));

    return all_replaced;
  }

  outcome::result<TokenAmount> MinerActorState::unlockUnvestedFunds(
      ChainEpoch curr_epoch, const TokenAmount &target) {
    if (target == 0 || this->locked_funds == 0) {
      return 0;
    }

    return v0::miner::MinerActorState::unlockUnvestedFunds(curr_epoch, target);
  }

  outcome::result<TokenAmount> MinerActorState::unlockVestedFunds(
      ChainEpoch curr_epoch) {
    if (this->locked_funds == 0) {
      return 0;
    }

    return v0::miner::MinerActorState::unlockVestedFunds(curr_epoch);
  }

  outcome::result<TokenAmount> MinerActorState::getUnlockedBalance(
      const TokenAmount &actor_balance) const {
    const TokenAmount unlocked_balance = actor_balance - this->locked_funds
                                         - this->precommit_deposit
                                         - this->initial_pledge;

    if (unlocked_balance < 0) {
      return ERROR_TEXT("negative unlocked balance");
    }

    return unlocked_balance;
  }

  outcome::result<TokenAmount> MinerActorState::getAvailableBalance(
      const TokenAmount &actor_balance) const {
    OUTCOME_TRY(unlocked_balance, getUnlockedBalance(actor_balance));
    return unlocked_balance - this->fee_debt;
  }

  outcome::result<void> MinerActorState::checkBalanceInvariants(
      const TokenAmount &balance) const {
    if (this->precommit_deposit < 0) {
      return ERROR_TEXT("pre-commit deposit is negative");
    }

    if (this->locked_funds < 0) {
      return ERROR_TEXT("locked funds is negative");
    }

    if (this->initial_pledge < 0) {
      return ERROR_TEXT("initial pledge is negative");
    }

    if (this->fee_debt < 0) {
      return ERROR_TEXT("fee debt is negative");
    }

    if (balance
        < this->precommit_deposit + this->locked_funds + this->initial_pledge) {
      return ERROR_TEXT("balance below required");
    }

    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::v2::miner
