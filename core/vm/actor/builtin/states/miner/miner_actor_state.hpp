/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "adt/cid_t.hpp"
#include "adt/map.hpp"
#include "adt/uvarint_key.hpp"
#include "common/outcome.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/types.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/builtin/types/miner/advance_deadline_result.hpp"
#include "vm/actor/builtin/types/miner/deadline_info.hpp"
#include "vm/actor/builtin/types/miner/deadline_sector_map.hpp"
#include "vm/actor/builtin/types/miner/deadlines.hpp"
#include "vm/actor/builtin/types/miner/miner_info.hpp"
#include "vm/actor/builtin/types/miner/quantize.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"
#include "vm/actor/builtin/types/miner/termination.hpp"
#include "vm/actor/builtin/types/miner/vesting.hpp"
#include "vm/actor/builtin/types/universal/universal.hpp"

// Forward declaration
namespace fc::vm::runtime {
  class Runtime;
}

namespace fc::vm::actor::builtin::states {
  using adt::UvarintKeyer;
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::TokenAmount;
  using runtime::Runtime;
  using types::Universal;
  using types::miner::AdvanceDeadlineResult;
  using types::miner::DeadlineInfo;
  using types::miner::Deadlines;
  using types::miner::DeadlineSectorMap;
  using types::miner::MinerInfo;
  using types::miner::PowerPair;
  using types::miner::QuantSpec;
  using types::miner::SectorOnChainInfo;
  using types::miner::SectorPreCommitOnChainInfo;
  using types::miner::Sectors;
  using types::miner::TerminationResult;
  using types::miner::VestingFunds;
  using types::miner::VestSpec;

  constexpr size_t kPrecommitExpiryBitwidth = 6;

  /**
   * Balance of Miner Actor should be greater than or equal to the sum of
   * PreCommitDeposits and LockedFunds. It is possible for balance to fall below
   * the sum of PCD, LF and InitialPledgeRequirements, and this is a bad
   * state (IP Debt) that limits a miner actor's behavior (i.e. no balance
   * withdrawals) Excess balance as computed by st.GetAvailableBalance will be
   * withdrawable or usable for pre-commit deposit <or> pledge lock-up.
   */
  struct MinerActorState {
    virtual ~MinerActorState() = default;

    /** Information not related to sectors. */
    adt::CbCidT<Universal<MinerInfo>> miner_info;

    /** Total funds locked as PreCommitDeposits */
    TokenAmount precommit_deposit;

    /** Total rewards and added funds locked in vesting table */
    TokenAmount locked_funds;

    /** VestingFunds (Vesting Funds schedule for the miner). */
    adt::CbCidT<VestingFunds> vesting_funds;

    /**
     * Absolute value of debt this miner owes from unpaid fees
     */
    TokenAmount fee_debt;

    /** Sum of initial pledge requirements of all active sectors */
    TokenAmount initial_pledge;

    /**
     * Sectors that have been pre-committed but not yet proven
     * HAMT[SectorNumber -> SectorPreCommitOnChainInfo]
     */
    adt::Map<SectorPreCommitOnChainInfo, UvarintKeyer> precommitted_sectors;

    /**
     * Maintains the state required to expire PreCommittedSectors
     */
    adt::Array<RleBitset, kPrecommitExpiryBitwidth>
        precommitted_setctors_expiry;

    /**
     * Allocated sector IDs. Sector IDs can never be reused once allocated.
     * RleBitset
     */
    adt::CbCidT<RleBitset> allocated_sectors;

    /**
     * Information for all proven and not-yet-garbage-collected sectors.
     * Sectors are removed from this AMT when the partition to which the
     * sector belongs is compacted.
     */
    Sectors sectors;

    /**
     * The first epoch in this miner's current proving period. This is the first
     * epoch in which a PoSt for a partition at the miner's first deadline may
     * arrive. Alternatively, it is after the last epoch at which a PoSt for the
     * previous window is valid. Always greater than zero, this may be greater
     * than the current epoch for genesis miners in the first WPoStProvingPeriod
     * epochs of the chain; the epochs before the first proving period starts
     * are exempt from Window PoSt requirements. Updated at the end of every
     * period by a cron callback.
     */
    ChainEpoch proving_period_start{};

    /**
     * Index of the deadline within the proving period beginning at
     * ProvingPeriodStart that has not yet been finalized. Updated at the end of
     * each deadline window by a cron callback.
     */
    uint64_t current_deadline{};

    /**
     * The sector numbers due for PoSt at each deadline in the current proving
     * period, frozen at period start. New sectors are added and expired ones
     * removed at proving period boundary. Faults are not subtracted from this
     * in state, but on the fly.
     */
    adt::CbCidT<Deadlines> deadlines;

    /** Deadlines with outstanding fees for early sector termination. */
    RleBitset early_terminations;

    // Methods

    /**
     * Returns Miner Info
     * NOTE: use this method to get access to Miner Info,
     * don't use miner_info field directly
     */
    virtual outcome::result<Universal<MinerInfo>> getInfo() const = 0;

    inline DeadlineInfo deadlineInfo(ChainEpoch now) const {
      return DeadlineInfo(
          this->proving_period_start, this->current_deadline, now);
    }

    inline QuantSpec quantSpecForDeadline(uint64_t deadline_id) const {
      return DeadlineInfo(this->proving_period_start, deadline_id, 0).quant();
    }

    outcome::result<void> allocateSectorNumber(SectorNumber sector_num);

    outcome::result<void> maskSectorNumbers(const RleBitset &sector_nos);

    /**
     * This method gets and returns the requested pre-committed sectors,
     * skipping missing sectors.
     */
    outcome::result<std::vector<SectorPreCommitOnChainInfo>>
    findPrecommittedSectors(const std::vector<SectorNumber> &sector_nos);

    outcome::result<void> deletePrecommittedSectors(
        const std::vector<SectorNumber> &sector_nos);

    outcome::result<void> deleteSectors(const RleBitset &sector_nos);

    /**
     * Schedules each sector to expire at its next deadline end. If it can't
     * find any given sector, it skips it. This method assumes that each
     * sector's power has not changed, despite the rescheduling.
     *
     * NOTE: this method is used to "upgrade" sectors, rescheduling the
     * now-replaced sectors to expire at the end of the next deadline. Given the
     * expense of sealing a sector, this function skips
     * missing/faulty/terminated "upgraded" sectors instead of failing. That
     * way, the new sectors can still be proved.
     */
    virtual outcome::result<std::vector<SectorOnChainInfo>>
    rescheduleSectorExpirations(Runtime &runtime,
                                ChainEpoch curr_epoch,
                                SectorSize ssize,
                                const DeadlineSectorMap &deadline_sectors) = 0;

    /**
     * Assign new sectors to deadlines.
     */
    outcome::result<PowerPair> assignSectorsToDeadlines(
        Runtime &runtime,
        ChainEpoch curr_epoch,
        std::vector<SectorOnChainInfo> sectors_to_assign,
        uint64_t partition_size,
        SectorSize ssize);

    /**
     * Pops up to max early terminated sectors from all deadlines.
     * @return hasMore if we still have more early terminations to process.
     */
    outcome::result<std::tuple<TerminationResult, bool>> popEarlyTerminations(
        Runtime &runtime, uint64_t max_partitions, uint64_t max_sectors);

    /**
     * Returns an error if the target sector cannot be found and/or is
     * faulty/terminated.
     */
    outcome::result<void> checkSectorHealth(uint64_t deadline_id,
                                            uint64_t partition_id,
                                            SectorNumber sector) const;

    outcome::result<void> addPreCommitDeposit(const TokenAmount &amount);

    outcome::result<void> addInitialPledge(const TokenAmount &amount);

    /**
     * AddLockedFunds first vests and unlocks the vested funds AND then locks
     * the given funds in the vesting table.
     */
    outcome::result<TokenAmount> addLockedFunds(ChainEpoch curr_epoch,
                                                const TokenAmount &vesting_sum,
                                                const VestSpec &spec);

    /**
     * PenalizeFundsInPriorityOrder first unlocks unvested funds from the
     * vesting table. If the target is not yet hit it deducts funds from the
     * (new) available balance. Returns the amount unlocked from the vesting
     * table and the amount taken from current balance. If the penalty exceeds
     * the total amount available in the vesting table and unlocked funds the
     * penalty is reduced to match.
     */
    outcome::result<std::tuple<TokenAmount, TokenAmount>>
    penalizeFundsInPriorityOrder(ChainEpoch curr_epoch,
                                 const TokenAmount &target,
                                 const TokenAmount &unlocked_balance);

    /**
     * ApplyPenalty adds the provided penalty to fee debt.
     */
    outcome::result<void> applyPenalty(const TokenAmount &penalty);

    /**
     * Draws from vesting table and unlocked funds to repay up to the fee debt.
     * Returns the amount unlocked from the vesting table and the amount taken
     * from current balance. If the fee debt exceeds the total amount available
     * for repayment the fee debt field is updated to track the remaining debt.
     * Otherwise it is set to zero.
     */
    outcome::result<std::tuple<TokenAmount, TokenAmount>>
    repayPartialDebtInPriorityOrder(ChainEpoch curr_epoch,
                                    const TokenAmount &curr_balance);

    /**
     * Repays the full miner actor fee debt.  Returns the amount that must be
     * burnt and an error if there are not sufficient funds to cover repayment.
     * Miner state repays from unlocked funds and fails if unlocked funds are
     * insufficient to cover fee debt. FeeDebt will be zero after a successful
     * call.
     */
    outcome::result<TokenAmount> repayDebts(const TokenAmount &curr_balance);

    /**
     * Unlocks an amount of funds that have *not yet vested*, if possible.
     * The soonest-vesting entries are unlocked first.
     * @return the amount actually unlocked.
     */
    virtual outcome::result<TokenAmount> unlockUnvestedFunds(
        ChainEpoch curr_epoch, const TokenAmount &target) = 0;

    /**
     * Unlocks all vesting funds that have vested before the provided epoch.
     * @return the amount unlocked.
     */
    virtual outcome::result<TokenAmount> unlockVestedFunds(
        ChainEpoch curr_epoch) = 0;

    /**
     * @return the amount of vested funds that have vested before the provided
     * epoch.
     */
    outcome::result<TokenAmount> checkVestedFunds(ChainEpoch curr_epoch) const;

    virtual outcome::result<TokenAmount> getUnlockedBalance(
        const TokenAmount &actor_balance) const = 0;

    virtual outcome::result<TokenAmount> getAvailableBalance(
        const TokenAmount &actor_balance) const = 0;

    virtual outcome::result<void> checkBalanceInvariants(
        const TokenAmount &balance) const = 0;

    outcome::result<bool> meetsInitialPledgeCondition(
        const TokenAmount &balance) const;

    bool isDebtFree() const;

    QuantSpec quantSpecEveryDeadline() const;

    outcome::result<void> addPreCommitExpiry(ChainEpoch expire_epoch,
                                             SectorNumber sector_num);

    outcome::result<TokenAmount> checkPrecommitExpiry(
        const RleBitset &sectors_to_check);

    outcome::result<TokenAmount> expirePreCommits(ChainEpoch curr_epoch);

    /**
     * AdvanceDeadline advances the deadline. It:
     * - Processes expired sectors.
     * - Handles missed proofs.
     * @return the changes to power & pledge, and faulty power (both declared
     * and undeclared).
     */
    outcome::result<AdvanceDeadlineResult> advanceDeadline(
        Runtime &runtime, ChainEpoch curr_epoch);
  };

  using MinerActorStatePtr = Universal<MinerActorState>;

  outcome::result<MinerActorStatePtr> makeEmptyMinerState(
      const IpldPtr &runtime);

}  // namespace fc::vm::actor::builtin::states
