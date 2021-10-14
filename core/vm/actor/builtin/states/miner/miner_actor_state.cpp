/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"

#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"
#include "vm/actor/builtin/types/miner/deadline_assignment.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::states {
  using namespace types::miner;

  outcome::result<void> MinerActorState::allocateSectorNumber(
      SectorNumber sector_num) {
    if (sector_num > kMaxSectorNumber) {
      return VMExitCode::kErrIllegalArgument;
    }

    OUTCOME_TRY(allocated, this->allocated_sectors.get());

    if (allocated.has(sector_num)) {
      return VMExitCode::kErrIllegalArgument;
    }

    allocated.insert(sector_num);

    OUTCOME_TRY(this->allocated_sectors.set(allocated));

    return outcome::success();
  }

  outcome::result<void> MinerActorState::maskSectorNumbers(
      const RleBitset &sector_nos) {
    if (sector_nos.empty()) {
      return VMExitCode::kErrIllegalArgument;
    }

    const auto &last_sector = *std::prev(sector_nos.end());

    if (last_sector > kMaxSectorNumber) {
      return VMExitCode::kErrIllegalArgument;
    }

    OUTCOME_TRY(allocated, this->allocated_sectors.get());

    allocated += sector_nos;
    OUTCOME_TRY(this->allocated_sectors.set(allocated));

    return outcome::success();
  }

  outcome::result<std::vector<SectorPreCommitOnChainInfo>>
  MinerActorState::findPrecommittedSectors(
      const std::vector<SectorNumber> &sector_nos) {
    std::vector<SectorPreCommitOnChainInfo> result;

    for (const auto &sector : sector_nos) {
      OUTCOME_TRY(maybe_info, this->precommitted_sectors.tryGet(sector));
      if (!maybe_info.has_value()) {
        continue;
      }
      result.push_back(maybe_info.value());
    }

    return result;
  }

  outcome::result<void> MinerActorState::deletePrecommittedSectors(
      const std::vector<SectorNumber> &sector_nos) {
    for (const auto &sector : sector_nos) {
      OUTCOME_TRY(this->precommitted_sectors.remove(sector));
    }

    return outcome::success();
  }

  outcome::result<void> MinerActorState::deleteSectors(
      const RleBitset &sector_nos) {
    for (const auto &sector : sector_nos) {
      OUTCOME_TRY(this->sectors.sectors.remove(sector));
    }

    return outcome::success();
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
                assignDeadlines(kMaxPartitionsPerDeadline,
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

  outcome::result<std::tuple<TerminationResult, bool>>
  MinerActorState::popEarlyTerminations(Runtime &runtime,
                                        uint64_t max_partitions,
                                        uint64_t max_sectors) {
    if (this->early_terminations.empty()) {
      return std::make_tuple(TerminationResult{}, false);
    }

    TerminationResult termination_result;

    OUTCOME_TRY(dls, this->deadlines.get());

    for (const auto &deadline_id : this->early_terminations) {
      OUTCOME_TRY(deadline, dls.loadDeadline(deadline_id));
      OUTCOME_TRY(result,
                  deadline->popEarlyTerminations(
                      runtime,
                      max_partitions - termination_result.partitions_processed,
                      max_sectors - termination_result.sectors_processed));
      const auto &[deadline_result, more] = result;

      termination_result.add(deadline_result);

      if (!more) {
        this->early_terminations.erase(deadline_id);
      }

      OUTCOME_TRY(dls.updateDeadline(deadline_id, deadline));

      if (!termination_result.belowLimit(max_partitions, max_sectors)) {
        break;
      }
    }

    OUTCOME_TRY(this->deadlines.set(dls));

    return std::make_tuple(termination_result,
                           !this->early_terminations.empty());
  }

  outcome::result<void> MinerActorState::checkSectorHealth(
      uint64_t deadline_id, uint64_t partition_id, SectorNumber sector) const {
    OUTCOME_TRY(dls, this->deadlines.get());
    OUTCOME_TRY(deadline, dls.loadDeadline(deadline_id));
    OUTCOME_TRY(partition, deadline->partitions.get(partition_id));

    if (!partition->sectors.has(sector)) {
      return VMExitCode::kErrNotFound;
    }

    if (partition->faults.has(sector)) {
      return VMExitCode::kErrForbidden;
    }

    if (partition->terminated.has(sector)) {
      return VMExitCode::kErrNotFound;
    }

    return outcome::success();
  }

  outcome::result<void> MinerActorState::addPreCommitDeposit(
      const TokenAmount &amount) {
    const auto new_total = this->precommit_deposit + amount;
    if (new_total < 0) {
      return ERROR_TEXT("negative pre-commit deposit after adding to prior");
    }

    this->precommit_deposit = new_total;
    return outcome::success();
  }

  outcome::result<void> MinerActorState::addInitialPledge(
      const TokenAmount &amount) {
    const auto new_total = this->initial_pledge + amount;
    if (new_total < 0) {
      return ERROR_TEXT("negative initial pledge after adding to prior");
    }

    this->initial_pledge = new_total;
    return outcome::success();
  }

  outcome::result<TokenAmount> MinerActorState::addLockedFunds(
      ChainEpoch curr_epoch,
      const TokenAmount &vesting_sum,
      const VestSpec &spec) {
    if (vesting_sum < 0) {
      return ERROR_TEXT("negative amount to lock");
    }

    OUTCOME_TRY(vf, this->vesting_funds.get());

    const auto amount_unlocked = vf.unlockVestedFunds(curr_epoch);
    this->locked_funds -= amount_unlocked;
    if (this->locked_funds < 0) {
      return ERROR_TEXT("negative locked funds after unlocking");
    }

    vf.addLockedFunds(
        curr_epoch, vesting_sum, this->proving_period_start, spec);
    this->locked_funds += vesting_sum;

    OUTCOME_TRY(this->vesting_funds.set(vf));

    return amount_unlocked;
  }

  outcome::result<std::tuple<TokenAmount, TokenAmount>>
  MinerActorState::penalizeFundsInPriorityOrder(
      ChainEpoch curr_epoch,
      const TokenAmount &target,
      const TokenAmount &unlocked_balance) {
    OUTCOME_TRY(from_vesting, unlockUnvestedFunds(curr_epoch, target));
    if (from_vesting == target) {
      return std::make_tuple(from_vesting, 0);
    }

    const TokenAmount remaining = target - from_vesting;
    const auto from_balance = std::min(unlocked_balance, remaining);
    return std::make_tuple(from_vesting, from_balance);
  }

  outcome::result<void> MinerActorState::applyPenalty(
      const TokenAmount &penalty) {
    if (penalty < 0) {
      return ERROR_TEXT("applying negative penalty not allowed");
    }

    this->fee_debt += penalty;
    return outcome::success();
  }

  outcome::result<std::tuple<TokenAmount, TokenAmount>>
  MinerActorState::repayPartialDebtInPriorityOrder(
      ChainEpoch curr_epoch, const TokenAmount &curr_balance) {
    OUTCOME_TRY(unlocked_balance, getUnlockedBalance(curr_balance));
    OUTCOME_TRY(from_vesting, unlockUnvestedFunds(curr_epoch, this->fee_debt));
    if (from_vesting > this->fee_debt) {
      return ERROR_TEXT("unlocked more vesting funds than required for debt");
    }

    this->fee_debt -= from_vesting;

    const auto from_balance = std::min(unlocked_balance, this->fee_debt);
    this->fee_debt -= from_balance;

    return std::make_tuple(from_vesting, from_balance);
  }

  outcome::result<TokenAmount> MinerActorState::repayDebts(
      const TokenAmount &curr_balance) {
    OUTCOME_TRY(unlocked_balance, getUnlockedBalance(curr_balance));
    if (unlocked_balance < this->fee_debt) {
      return ERROR_TEXT("unlocked balance can not repay fee debt");
    }

    const auto debt_to_repay = this->fee_debt;
    this->fee_debt = 0;
    return debt_to_repay;
  }

  outcome::result<TokenAmount> MinerActorState::checkVestedFunds(
      ChainEpoch curr_epoch) const {
    OUTCOME_TRY(vf, this->vesting_funds.get());

    TokenAmount amount_vested{0};

    for (const auto &fund : vf.funds) {
      if (fund.epoch >= curr_epoch) {
        break;
      }

      amount_vested += fund.amount;
    }

    return amount_vested;
  }

  outcome::result<bool> MinerActorState::meetsInitialPledgeCondition(
      const TokenAmount &balance) const {
    OUTCOME_TRY(available, getUnlockedBalance(balance));
    return available >= this->initial_pledge;
  }

  bool MinerActorState::isDebtFree() const {
    return this->fee_debt <= 0;
  }

  QuantSpec MinerActorState::quantSpecEveryDeadline() const {
    return QuantSpec(kWPoStChallengeWindow, this->proving_period_start);
  }

  outcome::result<void> MinerActorState::addPreCommitExpiry(
      ChainEpoch expire_epoch, SectorNumber sector_num) {
    const auto quant = quantSpecEveryDeadline();
    BitfieldQueue<kPrecommitExpiryBitwidth> queue{
        .queue = this->precommitted_setctors_expiry, .quant = quant};

    OUTCOME_TRY(queue.addToQueue(expire_epoch, {sector_num}));

    this->precommitted_setctors_expiry = queue.queue;
    return outcome::success();
  }

  outcome::result<TokenAmount> MinerActorState::checkPrecommitExpiry(
      const RleBitset &sectors_to_check) {
    TokenAmount deposit_to_burn{0};
    std::vector<SectorNumber> precommits_to_delete;

    for (const auto &sector_num : sectors_to_check) {
      OUTCOME_TRY(maybe_sector, precommitted_sectors.tryGet(sector_num));
      if (!maybe_sector.has_value()) {
        // already committed/deleted
        continue;
      }

      const auto &sector = maybe_sector.value();
      precommits_to_delete.push_back(sector_num);
      deposit_to_burn += sector.precommit_deposit;
    }

    if (!precommits_to_delete.empty()) {
      OUTCOME_TRY(deletePrecommittedSectors(precommits_to_delete));
    }

    this->precommit_deposit -= deposit_to_burn;
    if (this->precommit_deposit < 0) {
      return ERROR_TEXT("pre-commit expiry caused negative deposits");
    }

    return deposit_to_burn;
  }

  outcome::result<TokenAmount> MinerActorState::expirePreCommits(
      ChainEpoch curr_epoch) {
    BitfieldQueue<kPrecommitExpiryBitwidth> expiry_q{
        .queue = this->precommitted_setctors_expiry,
        .quant = quantSpecEveryDeadline()};

    OUTCOME_TRY(result, expiry_q.popUntil(curr_epoch));
    const auto &[popped_sectors, modified] = result;

    if (modified) {
      this->precommitted_setctors_expiry = expiry_q.queue;
    }

    return checkPrecommitExpiry(popped_sectors);
  }

  outcome::result<AdvanceDeadlineResult> MinerActorState::advanceDeadline(
      Runtime &runtime, ChainEpoch curr_epoch) {
    TokenAmount pledge_delta{0};
    PowerPair power_delta;
    PowerPair total_faulty_power;
    PowerPair detected_faulty_power;

    const auto dl_info = deadlineInfo(curr_epoch);
    if (!dl_info.periodStarted()) {
      return AdvanceDeadlineResult{};
    }

    this->current_deadline =
        (this->current_deadline + 1) % kWPoStPeriodDeadlines;
    if (this->current_deadline == 0) {
      this->proving_period_start += kWPoStProvingPeriod;
    }

    OUTCOME_TRY(dls, this->deadlines.get());
    OUTCOME_TRY(deadline, dls.loadDeadline(dl_info.index));

    const auto previously_faulty_power = deadline->faulty_power;

    if (deadline->live_sectors == 0) {
      return AdvanceDeadlineResult{
          .pledge_delta = pledge_delta,
          .power_delta = power_delta,
          .previously_faulty_power = previously_faulty_power,
          .detected_faulty_power = detected_faulty_power,
          .total_faulty_power = deadline->faulty_power};
    }

    const auto quant = quantSpecEveryDeadline();
    {
      const auto fault_expiration = dl_info.last() + kFaultMaxAge;
      OUTCOME_TRY(
          result,
          deadline->processDeadlineEnd(runtime, quant, fault_expiration));
      std::tie(power_delta, detected_faulty_power) = result;
      total_faulty_power = deadline->faulty_power;
    }
    {
      OUTCOME_TRY(expired,
                  deadline->popExpiredSectors(runtime, dl_info.last(), quant));
      pledge_delta -= expired.on_time_pledge;
      OUTCOME_TRY(addInitialPledge(-expired.on_time_pledge));
      power_delta -= expired.active_power;

      if (expired.early_sectors.empty()) {
        this->early_terminations.insert(dl_info.index);
      }
    }

    OUTCOME_TRY(dls.updateDeadline(dl_info.index, deadline));
    OUTCOME_TRY(this->deadlines.set(dls));

    return AdvanceDeadlineResult{
        .pledge_delta = pledge_delta,
        .power_delta = power_delta,
        .previously_faulty_power = previously_faulty_power,
        .detected_faulty_power = detected_faulty_power,
        .total_faulty_power = total_faulty_power};
  }

  outcome::result<MinerActorStatePtr> makeEmptyMinerState(
      const Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    const auto ipld = runtime.getIpfsDatastore();

    MinerActorStatePtr state{version};
    cbor_blake::cbLoadT(ipld, state);

    if (version < ActorVersion::kVersion3) {
      // Lotus gas conformance - flush empty hamt
      OUTCOME_TRY(state->precommitted_sectors.hamt.flush());

      // Lotus gas conformance - flush empty amt
      OUTCOME_TRY(empty_amt_cid,
                  state->precommitted_setctors_expiry.amt.flush());

      RleBitset allocated_sectors;
      OUTCOME_TRY(state->allocated_sectors.set(allocated_sectors));

      // construct with empty already cid stored in ipld to avoid gas charge
      state->sectors.sectors = {empty_amt_cid, ipld};

      OUTCOME_TRY(deadlines, makeEmptyDeadlines(runtime, empty_amt_cid));
      OUTCOME_TRY(state->deadlines.set(deadlines));

      VestingFunds vesting_funds;
      OUTCOME_TRY(state->vesting_funds.set(vesting_funds));
    } else {
      // Lotus gas conformance - flush empty hamt
      OUTCOME_TRY(state->precommitted_sectors.hamt.flush());

      // Lotus gas conformance - flush empty amt
      OUTCOME_TRY(state->precommitted_setctors_expiry.amt.flush());

      RleBitset allocated_sectors;
      OUTCOME_TRY(state->allocated_sectors.set(allocated_sectors));

      // Lotus gas conformance - flush empty amt
      OUTCOME_TRY(state->sectors.sectors.amt.flush());

      OUTCOME_TRY(deadlines, makeEmptyDeadlines(runtime, kEmptyObjectCid));
      OUTCOME_TRY(state->deadlines.set(deadlines));

      VestingFunds vesting_funds;
      OUTCOME_TRY(state->vesting_funds.set(vesting_funds));
    }

    return state;
  }

}  // namespace fc::vm::actor::builtin::states
