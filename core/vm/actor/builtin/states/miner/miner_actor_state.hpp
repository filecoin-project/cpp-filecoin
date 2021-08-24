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
#include "vm/actor/builtin/types/miner/deadline_info.hpp"
#include "vm/actor/builtin/types/miner/deadlines.hpp"
#include "vm/actor/builtin/types/miner/miner_info.hpp"
#include "vm/actor/builtin/types/miner/vesting.hpp"
#include "vm/actor/builtin/types/type_manager/universal.hpp"

namespace fc::vm::actor::builtin::states {
  using adt::UvarintKeyer;
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::TokenAmount;
  using types::Universal;
  using types::miner::DeadlineInfo;
  using types::miner::Deadlines;
  using types::miner::MinerInfo;
  using types::miner::SectorOnChainInfo;
  using types::miner::SectorPreCommitOnChainInfo;
  using types::miner::VestingFunds;

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
    TokenAmount initial_pledge_requirement;

    /**
     * Sectors that have been pre-committed but not yet proven
     * HAMT[SectorNumber -> SectorPreCommitOnChainInfo]
     */
    adt::Map<SectorPreCommitOnChainInfo, UvarintKeyer> precommitted_sectors;

    /**
     * Maintains the state required to expire PreCommittedSectors
     */
    adt::Array<RleBitset, 6> precommitted_setctors_expiry;

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
    types::miner::SectorArray sectors;

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
     * Returns deadline calculations for the current (according to state)
     * proving period.
     * @param now - current chain epoch
     * @return deadline calculations
     */
    inline DeadlineInfo deadlineInfo(ChainEpoch now) const {
      return DeadlineInfo(proving_period_start, current_deadline, now);
    }

    virtual outcome::result<Universal<MinerInfo>> getInfo() const = 0;
  };

  using MinerActorStatePtr = Universal<MinerActorState>;

}  // namespace fc::vm::actor::builtin::states
