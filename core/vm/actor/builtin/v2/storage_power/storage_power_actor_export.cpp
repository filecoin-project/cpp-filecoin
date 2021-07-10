/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/storage_power/storage_power_actor_export.hpp"

#include "common/logger.hpp"
#include "vm/actor/builtin/states/power_actor_state.hpp"
#include "vm/actor/builtin/v2/init/init_actor.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor.hpp"
#include "vm/actor/builtin/v2/reward/reward_actor.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v2::storage_power {
  using adt::Multimap;
  using primitives::SectorNumber;
  using toolchain::Toolchain;
  using states::PowerActorStatePtr;
  using types::storage_power::CronEvent;

  outcome::result<void> processDeferredCronEvents(Runtime &runtime) {
    const auto now{runtime.getCurrentEpoch()};
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());
    REQUIRE_NO_ERROR(state->cron_event_queue.hamt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    REQUIRE_NO_ERROR(state->claims.hamt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    std::vector<CronEvent> cron_events;
    for (auto epoch = state->first_cron_epoch; epoch <= now; ++epoch) {
      REQUIRE_NO_ERROR_A(events,
                         Multimap::values(state->cron_event_queue, epoch),
                         VMExitCode::kErrIllegalState);
      for (auto &event : events) {
        REQUIRE_NO_ERROR_A(has_claim,
                           state->hasClaim(event.miner_address),
                           VMExitCode::kErrIllegalState);
        if (has_claim) {
          cron_events.push_back(event);
        }
      }
      if (!events.empty()) {
        REQUIRE_NO_ERROR(state->cron_event_queue.remove(epoch),
                         VMExitCode::kErrIllegalState);
      }
    }
    state->first_cron_epoch = now + 1;
    OUTCOME_TRY(runtime.commitState(state));
    // TODO: can one miner fail several times here?
    std::vector<Address> failed_miners;
    for (auto &event : cron_events) {
      OUTCOME_TRY(code,
                  asExitCode(runtime.send(event.miner_address,
                                          miner::OnDeferredCronEvent::Number,
                                          MethodParams{event.callback_payload},
                                          0)));
      if (code != VMExitCode::kOk) {
        failed_miners.push_back(event.miner_address);
      }
    }
    if (!failed_miners.empty()) {
      OUTCOME_TRYA(state, runtime.getActorState<PowerActorStatePtr>());
      for (auto &miner : failed_miners) {
        OUTCOME_TRY(state->deleteClaim(runtime, miner));
        if (runtime.getNetworkVersion() >= NetworkVersion::kVersion7) {
          --state->miner_count;
        }
      }
      OUTCOME_TRY(runtime.commitState(state));
    }
    return outcome::success();
  }

  outcome::result<void> processBatchProofVerifiers(Runtime &runtime) {
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());
    runtime::BatchSealsIn batch;
    const auto _batch{state->proof_validation_batch};
    state->proof_validation_batch.reset();
    if (_batch) {
      REQUIRE_NO_ERROR(_batch->hamt.loadRoot(), VMExitCode::kErrIllegalState);
      REQUIRE_NO_ERROR(state->claims.hamt.loadRoot(),
                       VMExitCode::kErrIllegalState);
      REQUIRE_NO_ERROR(_batch->visit([&](auto &miner, auto &_seals)
                                         -> outcome::result<void> {
        REQUIRE_NO_ERROR_A(
            has_claim, state->hasClaim(miner), VMExitCode::kErrIllegalState);
        if (has_claim) {
          OUTCOME_TRY(seals, _seals.values());
          batch.emplace_back(miner, std::move(seals));
        }
        return outcome::success();
      }),
                       VMExitCode::kErrIllegalState);
    }
    OUTCOME_TRY(runtime.commitState(state));
    REQUIRE_NO_ERROR_A(verified,
                       runtime.batchVerifySeals(batch),
                       VMExitCode::kErrIllegalState);
    for (const auto &[miner, successful] : verified) {
      OUTCOME_TRY(asExitCode(runtime.sendM<miner::ConfirmSectorProofsValid>(
          miner, {successful}, 0)));
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(CreateMiner) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    REQUIRE_NO_ERROR_A(miner_params,
                       encodeActorParams(miner::Construct::Params{
                           .owner = params.owner,
                           .worker = params.worker,
                           .control_addresses = {},
                           .seal_proof_type = params.seal_proof_type,
                           .peer_id = params.peer_id,
                           .multiaddresses = params.multiaddresses}),
                       VMExitCode::kErrIllegalState);

    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    REQUIRE_SUCCESS_A(
        addresses_created,
        runtime.sendM<init::Exec>(
            kInitAddress,
            {address_matcher->getStorageMinerCodeId(), miner_params},
            runtime.getValueReceived()));
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());
    REQUIRE_NO_ERROR(state->setClaim(runtime,
                                     addresses_created.id_address,
                                     0,
                                     0,
                                     params.seal_proof_type),
                     VMExitCode::kErrIllegalState);
    ++state->miner_count;
    OUTCOME_TRY(runtime.commitState(state));
    return Result{addresses_created.id_address,
                  addresses_created.robust_address};
  }

  ACTOR_METHOD_IMPL(OnEpochTickEnd) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kCronAddress));
    OUTCOME_TRY(processBatchProofVerifiers(runtime));
    OUTCOME_TRY(processDeferredCronEvents(runtime));
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());

    const auto [raw_power, qa_power] = state->getCurrentTotalPower();
    state->this_epoch_pledge_collateral = state->total_pledge_collateral;
    state->this_epoch_raw_power = raw_power;
    state->this_epoch_qa_power = qa_power;

    state->updateSmoothedEstimate(1);

    OUTCOME_TRY(runtime.commitState(state));
    REQUIRE_SUCCESS(runtime.sendM<reward::UpdateNetworkKPI>(
        kRewardAddress, state->this_epoch_raw_power, 0));
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<CreateMiner>(),
      exportMethod<UpdateClaimedPower>(),
      exportMethod<EnrollCronEvent>(),
      exportMethod<OnEpochTickEnd>(),
      exportMethod<UpdatePledgeTotal>(),
      exportMethod<SubmitPoRepForBulkVerify>(),
      exportMethod<CurrentTotalPower>(),
  };
}  // namespace fc::vm::actor::builtin::v2::storage_power
