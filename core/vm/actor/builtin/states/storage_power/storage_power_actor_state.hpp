/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <tuple>
#include "adt/address_key.hpp"
#include "adt/multimap.hpp"
#include "adt/uvarint_key.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/storage_power/claim.hpp"
#include "vm/actor/builtin/types/storage_power/cron_event.hpp"
#include "vm/actor/builtin/types/universal/universal.hpp"

// Forward declaration
namespace fc::vm::runtime {
  class Runtime;
}

namespace fc::vm::actor::builtin::states {
  using common::smoothing::FilterEstimate;
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::SealVerifyInfo;
  using ChainEpochKeyer = adt::VarintKeyer;
  using runtime::Runtime;
  using types::Universal;
  using types::storage_power::Claim;
  using types::storage_power::CronEvent;

  struct PowerActorState {
    PowerActorState();
    virtual ~PowerActorState() = default;

    StoragePower total_raw_power{};

    /** includes claims from miners below min power threshold */
    StoragePower total_raw_commited{};
    StoragePower total_qa_power{};

    /** includes claims from miners below min power threshold */
    StoragePower total_qa_commited{};
    TokenAmount total_pledge_collateral{};

    /**
     * These fields are set once per epoch in the previous cron tick and used
     * for consistent values across a single epoch's state transition.
     */
    StoragePower this_epoch_raw_power{};
    StoragePower this_epoch_qa_power{};
    TokenAmount this_epoch_pledge_collateral{};
    FilterEstimate this_epoch_qa_power_smoothed{};

    size_t miner_count{};
    size_t num_miners_meeting_min_power{};
    adt::Map<adt::Array<CronEvent, 6>, ChainEpochKeyer, 6> cron_event_queue;

    /**
     * First epoch in which a cron task may be stored.
     * Cron will iterate every epoch between this and the current epoch
     * inclusively to find tasks to execute.
     */
    ChainEpoch first_cron_epoch{};
    ChainEpoch last_processed_cron_epoch{};

    // Don't use these fields directly, use methods to manage claims
    adt::Map<Universal<Claim>, adt::AddressKeyer> claims;

    boost::optional<adt::Map<adt::Array<SealVerifyInfo, 4>, adt::AddressKeyer>>
        proof_validation_batch;

    // Methods
    outcome::result<void> addToClaim(const Runtime &runtime,
                                     const Address &address,
                                     const StoragePower &raw,
                                     const StoragePower &qa);

    outcome::result<void> setClaim(
        const Runtime &runtime,
        const Address &address,
        const StoragePower &raw,
        const StoragePower &qa,
        RegisteredSealProof seal_proof = RegisteredSealProof::kUndefined);

    virtual outcome::result<void> deleteClaim(const Runtime &runtime,
                                              const Address &address) = 0;

    outcome::result<bool> hasClaim(const Address &address) const;

    outcome::result<boost::optional<Universal<Claim>>> tryGetClaim(
        const Address &address) const;

    outcome::result<Universal<Claim>> getClaim(const Address &address) const;

    outcome::result<void> addPledgeTotal(const TokenAmount &amount);

    outcome::result<void> appendCronEvent(const ChainEpoch &epoch,
                                          const CronEvent &event);

    void updateSmoothedEstimate(int64_t delta);

    std::tuple<StoragePower, StoragePower> getCurrentTotalPower() const;

   protected:
    virtual std::tuple<bool, bool> claimsAreBelow(
        const Claim &old_claim, const Claim &new_claim) const = 0;
  };

  using PowerActorStatePtr = Universal<PowerActorState>;

}  // namespace fc::vm::actor::builtin::states
