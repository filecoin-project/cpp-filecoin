/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/state.hpp"

#include <tuple>
#include "adt/address_key.hpp"
#include "adt/multimap.hpp"
#include "adt/uvarint_key.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/storage_power/claim.hpp"
#include "vm/actor/builtin/types/storage_power/cron_event.hpp"

// Forward declaration
namespace fc::vm::runtime {
  class Runtime;
}

namespace fc::vm::actor::builtin::states {
  using common::Buffer;
  using common::smoothing::FilterEstimate;
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::SealVerifyInfo;
  using ChainEpochKeyer = adt::VarintKeyer;
  using types::storage_power::Claim;
  using types::storage_power::CronEvent;

  namespace storage_power {
    struct ClaimV0 : Claim {};
    CBOR_TUPLE(ClaimV0, raw_power, qa_power)

    struct ClaimV2 : Claim {};
    CBOR_TUPLE(ClaimV2, seal_proof_type, raw_power, qa_power)
  }  // namespace storage_power

  struct PowerActorState : State {
    PowerActorState();

    StoragePower total_raw_power{};

    /** includes claims from miners below min power threshold */
    StoragePower total_raw_commited{};
    StoragePower total_qa_power{};

    /** includes claims from miners below min power threshold */
    StoragePower total_qa_commited{};
    TokenAmount total_pledge{};

    /**
     * These fields are set once per epoch in the previous cron tick and used
     * for consistent values across a single epoch's state transition.
     */
    StoragePower this_epoch_raw_power{};
    StoragePower this_epoch_qa_power{};
    TokenAmount this_epoch_pledge{};
    FilterEstimate this_epoch_qa_power_smoothed{};

    size_t miner_count{};
    size_t num_miners_meeting_min_power{};
    adt::Map<adt::Array<CronEvent>, ChainEpochKeyer> cron_event_queue;

    /**
     * First epoch in which a cron task may be stored.
     * Cron will iterate every epoch between this and the current epoch
     * inclusively to find tasks to execute.
     */
    ChainEpoch first_cron_epoch{};
    ChainEpoch last_processed_cron_epoch{};

    // Don't use these fields directly, use methods to manage claims
    adt::Map<storage_power::ClaimV0, adt::AddressKeyer> claims0;
    adt::Map<storage_power::ClaimV2, adt::AddressKeyer> claims2;

    boost::optional<adt::Map<adt::Array<SealVerifyInfo>, adt::AddressKeyer>>
        proof_validation_batch;

    // Methods
    virtual std::shared_ptr<PowerActorState> copy() const = 0;

    outcome::result<void> addToClaim(const fc::vm::runtime::Runtime &runtime,
                                     const Address &address,
                                     const StoragePower &raw,
                                     const StoragePower &qa);

    virtual outcome::result<void> setClaim(
        const fc::vm::runtime::Runtime &runtime,
        const Address &address,
        const StoragePower &raw,
        const StoragePower &qa,
        RegisteredSealProof seal_proof =
            RegisteredSealProof::StackedDrg2KiBV1) = 0;

    virtual outcome::result<void> deleteClaim(
        const fc::vm::runtime::Runtime &runtime, const Address &address) = 0;

    virtual outcome::result<bool> hasClaim(const Address &address) const = 0;

    virtual outcome::result<boost::optional<Claim>> tryGetClaim(
        const Address &address) const = 0;

    virtual outcome::result<Claim> getClaim(const Address &address) const = 0;

    virtual outcome::result<std::vector<adt::AddressKeyer::Key>> getClaimsKeys()
        const = 0;

    virtual outcome::result<void> loadClaimsRoot() = 0;

    outcome::result<void> addPledgeTotal(
        const fc::vm::runtime::Runtime &runtime, const TokenAmount &amount);

    outcome::result<void> appendCronEvent(const ChainEpoch &epoch,
                                          const CronEvent &event);

    void updateSmoothedEstimate(int64_t delta);

    std::tuple<StoragePower, StoragePower> getCurrentTotalPower() const;

   protected:
    virtual std::tuple<bool, bool> claimsAreBelow(
        const Claim &old_claim, const Claim &new_claim) const = 0;
  };

  using PowerActorStatePtr = std::shared_ptr<PowerActorState>;
}  // namespace fc::vm::actor::builtin::states
