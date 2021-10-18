/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/storage_power/storage_power_actor_state.hpp"

#include "common/math/math.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "const.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::states {
  using adt::Multimap;
  using common::math::kPrecision128;
  using common::smoothing::nextEstimate;
  using primitives::BigInt;
  using primitives::kChainEpochUndefined;
  using runtime::Runtime;

  /** genesis power in bytes = 750,000 GiB */
  static const BigInt kInitialQAPowerEstimatePosition =
      BigInt(750000) * BigInt(1 << 30);

  /**
   * Max chain throughput in bytes per epoch = 120 ProveCommits / epoch =
   * 3,840 GiB
   */
  static const BigInt kInitialQAPowerEstimateVelocity =
      BigInt(3840) * BigInt(1 << 30);

  PowerActorState::PowerActorState()
      : this_epoch_qa_power_smoothed{.position = kInitialQAPowerEstimatePosition
                                                 << kPrecision128,
                                     .velocity = kInitialQAPowerEstimateVelocity
                                                 << kPrecision128},
        last_processed_cron_epoch(kChainEpochUndefined) {}

  outcome::result<void> PowerActorState::addToClaim(
      const fc::vm::runtime::Runtime &runtime,
      const Address &address,
      const StoragePower &raw,
      const StoragePower &qa) {
    OUTCOME_TRY(claim_found, tryGetClaim(address));
    if (!claim_found.has_value()) {
      return VMExitCode::kErrNotFound;
    }
    auto claim{**claim_found};

    // TotalBytes always update directly
    total_raw_commited += raw;
    total_qa_commited += qa;

    const auto old_claim{claim};
    claim.raw_power += raw;
    claim.qa_power += qa;

    const auto [prev_below, still_below] = claimsAreBelow(old_claim, claim);

    if (prev_below && !still_below) {
      ++num_miners_meeting_min_power;
      total_raw_power += claim.raw_power;
      total_qa_power += claim.qa_power;
    } else if (!prev_below && still_below) {
      --num_miners_meeting_min_power;
      total_raw_power -= old_claim.raw_power;
      total_qa_power -= old_claim.qa_power;
    } else if (!prev_below && !still_below) {
      total_raw_power += raw;
      total_qa_power += qa;
    }
    VM_ASSERT(claim.raw_power >= 0);
    VM_ASSERT(claim.qa_power >= 0);
    VM_ASSERT(num_miners_meeting_min_power >= 0);

    return setClaim(runtime,
                    address,
                    claim.raw_power,
                    claim.qa_power,
                    claim.seal_proof_type);
  }

  outcome::result<void> PowerActorState::setClaim(
      const Runtime &runtime,
      const Address &address,
      const StoragePower &raw,
      const StoragePower &qa,
      RegisteredSealProof seal_proof) {
    VM_ASSERT(raw >= 0);
    VM_ASSERT(qa >= 0);

    Universal<Claim> claim{runtime.getActorVersion()};
    claim->seal_proof_type = seal_proof;
    claim->raw_power = raw;
    claim->qa_power = qa;

    OUTCOME_TRY(claims.set(address, claim));
    return outcome::success();
  }

  outcome::result<bool> PowerActorState::hasClaim(
      const Address &address) const {
    return claims.has(address);
  }

  outcome::result<boost::optional<Universal<Claim>>>
  PowerActorState::tryGetClaim(const Address &address) const {
    OUTCOME_TRY(claim, claims.tryGet(address));
    if (claim) {
      return *claim;
    }
    return boost::none;
  }

  outcome::result<Universal<Claim>> PowerActorState::getClaim(
      const Address &address) const {
    OUTCOME_TRY(claim, claims.get(address));
    return std::move(claim);
  }

  outcome::result<void> PowerActorState::addPledgeTotal(
      const Runtime &runtime, const TokenAmount &amount) {
    total_pledge_collateral += amount;
    VM_ASSERT(total_pledge_collateral >= 0);
    return outcome::success();
  }

  outcome::result<void> PowerActorState::appendCronEvent(
      const ChainEpoch &epoch, const CronEvent &event) {
    if (epoch < first_cron_epoch) {
      first_cron_epoch = epoch;
    }
    return Multimap::append(cron_event_queue, epoch, event);
  }

  void PowerActorState::updateSmoothedEstimate(int64_t delta) {
    this_epoch_qa_power_smoothed =
        nextEstimate(this_epoch_qa_power_smoothed, this_epoch_qa_power, delta);
  }

  std::tuple<StoragePower, StoragePower> PowerActorState::getCurrentTotalPower()
      const {
    if (num_miners_meeting_min_power < kConsensusMinerMinMiners) {
      return std::make_tuple(total_raw_commited, total_qa_commited);
    }
    return std::make_tuple(total_raw_power, total_qa_power);
  }

}  // namespace fc::vm::actor::builtin::states
