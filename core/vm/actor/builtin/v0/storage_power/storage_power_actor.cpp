/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/storage_power/storage_power_actor.hpp"

#include "common/logger.hpp"
#include "vm/actor/builtin/states/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/v0/init/init_actor.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
  using adt::Multimap;
  using primitives::SectorNumber;
  using states::PowerActorStatePtr;
  using toolchain::Toolchain;
  using types::storage_power::CronEvent;
  using types::storage_power::kGasOnSubmitVerifySeal;
  using types::storage_power::kMaxMinerProveCommitsPerEpoch;

  outcome::result<void> processDeferredCronEvents(Runtime &runtime) {
    const auto now{runtime.getCurrentEpoch()};
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());
    REQUIRE_NO_ERROR(state->cron_event_queue.hamt.loadRoot(),
                     VMExitCode::kErrIllegalState);
    std::vector<CronEvent> cron_events;
    for (auto epoch = state->first_cron_epoch; epoch <= now; ++epoch) {
      REQUIRE_NO_ERROR_A(events,
                         Multimap::values(state->cron_event_queue, epoch),
                         VMExitCode::kErrIllegalState);
      cron_events.insert(cron_events.end(), events.begin(), events.end());
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
        OUTCOME_TRY(claim, state->tryGetClaim(miner));
        if (claim) {
          OUTCOME_TRY(state->addToClaim(
              runtime, miner, -(**claim).raw_power, -(**claim).qa_power));
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
      REQUIRE_NO_ERROR(
          _batch->visit(
              [&](auto &miner, auto &_seals) -> outcome::result<void> {
                OUTCOME_TRY(seals, _seals.values());
                batch.emplace_back(miner, std::move(seals));
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

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));

    PowerActorStatePtr state{runtime.getActorVersion()};
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(), state);

    OUTCOME_TRY(runtime.commitState(state));
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
                       VMExitCode::kErrSerialization);

    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    REQUIRE_SUCCESS_A(
        addresses_created,
        runtime.sendM<init::Exec>(
            kInitAddress,
            {address_matcher->getStorageMinerCodeId(), miner_params},
            runtime.getValueReceived()));
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());
    REQUIRE_NO_ERROR(
        state->setClaim(runtime, addresses_created.id_address, 0, 0),
        VMExitCode::kErrIllegalState);
    ++state->miner_count;
    OUTCOME_TRY(runtime.commitState(state));
    return Result{addresses_created.id_address,
                  addresses_created.robust_address};
  }

  ACTOR_METHOD_IMPL(UpdateClaimedPower) {
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateImmediateCallerType(
        address_matcher->getStorageMinerCodeId()));
    const Address miner_address = runtime.getImmediateCaller();
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());
    REQUIRE_NO_ERROR(state->addToClaim(runtime,
                                       miner_address,
                                       params.raw_byte_delta,
                                       params.quality_adjusted_delta),
                     VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(EnrollCronEvent) {
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateImmediateCallerType(
        address_matcher->getStorageMinerCodeId()));
    VALIDATE_ARG(params.event_epoch >= 0);
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());
    REQUIRE_NO_ERROR(
        state->appendCronEvent(params.event_epoch,
                               {.miner_address = runtime.getImmediateCaller(),
                                .callback_payload = params.payload}),
        VMExitCode::kErrIllegalState);
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(OnEpochTickEnd) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kCronAddress));
    OUTCOME_TRY(processDeferredCronEvents(runtime));
    OUTCOME_TRY(processBatchProofVerifiers(runtime));
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());

    const auto [raw_power, qa_power] = state->getCurrentTotalPower();
    state->this_epoch_pledge_collateral = state->total_pledge_collateral;
    state->this_epoch_raw_power = raw_power;
    state->this_epoch_qa_power = qa_power;

    const auto now{runtime.getCurrentEpoch()};
    const auto delta = now - state->last_processed_cron_epoch;
    state->updateSmoothedEstimate(delta);

    state->last_processed_cron_epoch = now;

    OUTCOME_TRY(runtime.commitState(state));
    REQUIRE_SUCCESS(runtime.sendM<reward::UpdateNetworkKPI>(
        kRewardAddress, state->this_epoch_raw_power, 0));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(UpdatePledgeTotal) {
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateImmediateCallerType(
        address_matcher->getStorageMinerCodeId()));
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());

    const auto utils = Toolchain::createPowerUtils(runtime);
    OUTCOME_TRY(
        utils->validateMinerHasClaim(state, runtime.getImmediateCaller()));

    OUTCOME_TRY(state->addPledgeTotal(params));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(OnConsensusFault) {
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateImmediateCallerType(
        address_matcher->getStorageMinerCodeId()));
    const auto miner{runtime.getImmediateCaller()};
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());
    REQUIRE_NO_ERROR_A(
        found_claim, state->tryGetClaim(miner), VMExitCode::kErrIllegalState);
    if (!found_claim.has_value()) {
      return VMExitCode::kErrNotFound;
    }
    const auto claim{found_claim.value()};
    VM_ASSERT(claim->raw_power >= 0);
    VM_ASSERT(claim->qa_power >= 0);
    REQUIRE_NO_ERROR(
        state->addToClaim(runtime, miner, -claim->raw_power, -claim->qa_power),
        VMExitCode::kErrIllegalState);
    OUTCOME_TRY(state->addPledgeTotal(-params));
    REQUIRE_NO_ERROR(state->deleteClaim(runtime, miner),
                     VMExitCode::kErrIllegalState);
    --state->miner_count;
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(SubmitPoRepForBulkVerify) {
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateImmediateCallerType(
        address_matcher->getStorageMinerCodeId()));
    const auto miner{runtime.getImmediateCaller()};
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());

    const auto utils = Toolchain::createPowerUtils(runtime);
    OUTCOME_TRY(
        utils->validateMinerHasClaim(state, runtime.getImmediateCaller()));

    if (!state->proof_validation_batch.has_value()) {
      state->proof_validation_batch.emplace(runtime.getIpfsDatastore());
    }
    REQUIRE_NO_ERROR_A(found,
                       state->proof_validation_batch->tryGet(miner),
                       VMExitCode::kErrIllegalState);
    if (found.has_value()) {
      OUTCOME_TRY(size, found.value().size());
      if (size >= kMaxMinerProveCommitsPerEpoch) {
        ABORT(kErrTooManyProveCommits);
      }
    }
    REQUIRE_NO_ERROR(
        Multimap::append(state->proof_validation_batch.value(), miner, params),
        VMExitCode::kErrIllegalState);

    // Lotus gas conformance
    OUTCOME_TRY(state->proof_validation_batch->hamt.flush());

    OUTCOME_TRY(runtime.chargeGas(kGasOnSubmitVerifySeal));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(CurrentTotalPower) {
    OUTCOME_TRY(state, runtime.getActorState<PowerActorStatePtr>());
    return Result{
        .raw_byte_power = state->this_epoch_raw_power,
        .quality_adj_power = state->this_epoch_qa_power,
        .pledge_collateral = state->this_epoch_pledge_collateral,
        .quality_adj_power_smoothed = state->this_epoch_qa_power_smoothed};
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<CreateMiner>(),
      exportMethod<UpdateClaimedPower>(),
      exportMethod<EnrollCronEvent>(),
      exportMethod<OnEpochTickEnd>(),
      exportMethod<UpdatePledgeTotal>(),
      exportMethod<OnConsensusFault>(),
      exportMethod<SubmitPoRepForBulkVerify>(),
      exportMethod<CurrentTotalPower>(),
  };
}  // namespace fc::vm::actor::builtin::v0::storage_power
