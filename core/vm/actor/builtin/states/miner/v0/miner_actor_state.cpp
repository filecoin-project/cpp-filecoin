/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/miner/v0/miner_actor_state.hpp"

#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"
#include "vm/actor/builtin/types/miner/deadline_assignment_heap.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v0::miner {
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

    for (const auto &[dl_id, pm] : deadline_sectors.map) {
      const auto dl_info =
          DeadlineInfo(this->proving_period_start, dl_id, curr_epoch)
              .nextNotElapsed();
      const auto new_expiration = dl_info.last();

      OUTCOME_TRY(deadline, dls.loadDeadline(dl_id));
      OUTCOME_TRY(deadline->rescheduleSectorExpirations(
          runtime, sectors, new_expiration, pm, ssize, dl_info.quant()));
      OUTCOME_TRY(dls.updateDeadline(dl_id, deadline));
    }

    OUTCOME_TRY(this->deadlines.set(dls));

    return std::vector<SectorOnChainInfo>{};
  }

  outcome::result<PowerPair> MinerActorState::assignSectorsToDeadlines(
      Runtime &runtime,
      ChainEpoch curr_epoch,
      std::vector<SectorOnChainInfo> sectors_to_assign,
      uint64_t partition_size,
      SectorSize ssize) {
    OUTCOME_TRY(dls, deadlines.get());

    std::sort(sectors_to_assign.begin(),
              sectors_to_assign.end(),
              [](const SectorOnChainInfo &lhs, const SectorOnChainInfo &rhs) {
                return lhs.sector < rhs.sector;
              });

    std::map<uint64_t, Universal<Deadline>> deadlines_to_assign;
    for (uint64_t dl_id = 0; dl_id < dls.due.size(); dl_id++) {
      OUTCOME_TRY(deadline, dls.loadDeadline(dl_id));

      // Skip deadlines that aren't currently mutable.
      if (deadlineIsMutable(this->proving_period_start, dl_id, curr_epoch)) {
        deadlines_to_assign[dl_id] = deadline;
      }
    }

    PowerPair activated_power;
    OUTCOME_TRY(deadline_to_sectors,
                assignDeadlines(getMaxPartitionsForDeadlineAssignment(),
                                partition_size,
                                deadlines_to_assign,
                                sectors_to_assign));
    for (uint64_t dl_id = 0; dl_id < deadline_to_sectors.size(); dl_id++) {
      const auto &deadline_sectors = deadline_to_sectors[dl_id];
      if (deadline_sectors.empty()) {
        continue;
      }

      const auto quant = quantSpecForDeadline(dl_id);
      auto &deadline = deadlines_to_assign[dl_id];

      OUTCOME_TRY(
          deadline_activated_power,
          deadline->addSectors(
              runtime, partition_size, false, deadline_sectors, ssize, quant));
      activated_power += deadline_activated_power;

      OUTCOME_TRY(dls.updateDeadline(dl_id, deadline));
    }

    OUTCOME_TRY(this->deadlines.set(dls));

    return activated_power;
  }

  outcome::result<TokenAmount> MinerActorState::unlockUnvestedFunds(
      ChainEpoch curr_epoch, const TokenAmount &target) {
    OUTCOME_TRY(vf, this->vesting_funds.get());

    const auto amount_unlocked = vf.unlockUnvestedFunds(curr_epoch, target);
    this->locked_funds -= amount_unlocked;
    if (this->locked_funds < 0) {
      return ERROR_TEXT("negative locked funds after unlocking");
    }

    OUTCOME_TRY(this->vesting_funds.set(vf));

    return amount_unlocked;
  }

  outcome::result<TokenAmount> MinerActorState::unlockVestedFunds(
      ChainEpoch curr_epoch) {
    OUTCOME_TRY(vf, this->vesting_funds.get());

    const auto amount_unlocked = vf.unlockVestedFunds(curr_epoch);
    this->locked_funds -= amount_unlocked;
    if (this->locked_funds < 0) {
      return ERROR_TEXT("vesting cause locked funds negative");
    }

    OUTCOME_TRY(this->vesting_funds.set(vf));

    return amount_unlocked;
  }

  outcome::result<TokenAmount> MinerActorState::getUnlockedBalance(
      const TokenAmount &actor_balance) const {
    const TokenAmount unlocked_balance =
        actor_balance - this->locked_funds - this->precommit_deposit;

    if (unlocked_balance < 0) {
      return ERROR_TEXT("negative unlocked balance");
    }

    return unlocked_balance;
  }

  outcome::result<TokenAmount> MinerActorState::getAvailableBalance(
      const TokenAmount &actor_balance) const {
    OUTCOME_TRY(unlocked_balance, getUnlockedBalance(actor_balance));
    return unlocked_balance - this->initial_pledge;
  }

  outcome::result<void> MinerActorState::checkBalanceInvariants(
      const TokenAmount &balance) const {
    if (this->precommit_deposit < 0) {
      return ERROR_TEXT("pre-commit deposit is negative");
    }

    if (this->locked_funds < 0) {
      return ERROR_TEXT("locked funds is negative");
    }

    if (balance < this->precommit_deposit + this->locked_funds) {
      return ERROR_TEXT("balance below required");
    }

    return outcome::success();
  }

  uint64_t MinerActorState::getMaxPartitionsForDeadlineAssignment() const {
    return 0;
  }

}  // namespace fc::vm::actor::builtin::v0::miner
