/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"

#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::states {
  using namespace types::miner;

  outcome::result<void> MinerActorState::allocateSectorNumber(
      SectorNumber sector_num) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<void> MinerActorState::maskSectorNumbers(
      const RleBitset &sector_nos) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<std::vector<SectorPreCommitOnChainInfo>>
  MinerActorState::findPrecommittedSectors(
      const std::vector<SectorNumber> &sector_nos) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<void> MinerActorState::deletePrecommittedSectors(
      const std::vector<SectorNumber> &sector_nos) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<void> MinerActorState::deleteSectors(
      const RleBitset &sector_nos) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<std::tuple<TerminationResult, bool>>
  MinerActorState::popEarlyTerminations(Runtime &runtime,
                                        uint64_t max_partitions,
                                        uint64_t max_sectors) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<void> MinerActorState::checkSectorHealth(
      uint64_t deadline_id, uint64_t partition_id, SectorNumber sector) const {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<void> MinerActorState::addPreCommitDeposit(
      const TokenAmount &amount) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<void> MinerActorState::addInitialPledge(
      const TokenAmount &amount) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<TokenAmount> MinerActorState::addLockedFunds(
      ChainEpoch curr_epoch,
      const TokenAmount &vesting_sum,
      const VestSpec &spec) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<std::tuple<TokenAmount, TokenAmount>>
  MinerActorState::penalizeFundsInPriorityOrder(
      ChainEpoch curr_epoch,
      const TokenAmount &target,
      const TokenAmount &unlocked_balance) const {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<void> MinerActorState::applyPenalty(
      const TokenAmount &penalty) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<std::tuple<TokenAmount, TokenAmount>>
  MinerActorState::repayPartialDebtInPriorityOrder(
      ChainEpoch curr_epoch, const TokenAmount &curr_balance) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<TokenAmount> MinerActorState::repayDebts(
      const TokenAmount &curr_balance) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<TokenAmount> MinerActorState::checkVestedFunds(
      ChainEpoch curr_epoch) const {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<bool> MinerActorState::meetsInitialPledgeCondition(
      const TokenAmount &balance) const {
    // todo
    return ERROR_TEXT("");
  }

  bool MinerActorState::isDebtFree() const {
    // todo
    return false;
  }

  QuantSpec MinerActorState::quantSpecEveryDeadline() const {
    // todo
    return QuantSpec{0, 0};
  }

  outcome::result<void> MinerActorState::addPreCommitExpiry(
      ChainEpoch expire_epoch, SectorNumber sector_num) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<TokenAmount> MinerActorState::checkPrecommitExpiry(
      const RleBitset &sectors_to_check) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<TokenAmount> MinerActorState::expirePreCommits(
      ChainEpoch curr_epoch) {
    // todo
    return ERROR_TEXT("");
  }

  outcome::result<AdvanceDeadlineResult> MinerActorState::advanceDeadline(
      Runtime &runtime, ChainEpoch curr_epoch) {
    // todo
    return ERROR_TEXT("");
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
    }

    return state;
  }

}  // namespace fc::vm::actor::builtin::states
