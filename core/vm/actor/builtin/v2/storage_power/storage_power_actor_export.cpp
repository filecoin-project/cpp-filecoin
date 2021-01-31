/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/storage_power/storage_power_actor_export.hpp"
#include "common/logger.hpp"
#include "vm/actor/builtin/v2/codes.hpp"
#include "vm/actor/builtin/v2/init/init_actor.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor.hpp"
#include "vm/actor/builtin/v2/reward/reward_actor.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::storage_power {
  using adt::Multimap;
  using primitives::SectorNumber;
  using v0::storage_power::kErrTooManyProveCommits;
  using v0::storage_power::kGasOnSubmitVerifySeal;
  using v0::storage_power::kMaxMinerProveCommitsPerEpoch;

  outcome::result<void> processDeferredCronEvents(Runtime &runtime,
                                                  State &state) {
    const auto now{runtime.getCurrentEpoch()};
    for (auto epoch = state.first_cron_epoch; epoch <= now; ++epoch) {
      OUTCOME_TRY(events, state.cron_event_queue.tryGet(epoch));
      if (events) {
        OUTCOME_TRY(events->visit([&](auto,
                                      auto &event) -> outcome::result<void> {
          // refuse to process proofs for miner with no claim
          OUTCOME_TRY(has_claim, state.claims.has(event.miner_address));
          if (!has_claim) {
            spdlog::warn("skipping batch verifies for unknown miner {}",
                         encodeToString(event.miner_address));
            return outcome::success();
          }

          const auto res{runtime.send(event.miner_address,
                                      miner::OnDeferredCronEvent::Number,
                                      MethodParams{event.callback_payload},
                                      0)};
          if (!res) {
            spdlog::warn(
                "PowerActor.processDeferredCronEvents: error {} \"{}\", epoch "
                "{}, miner {}, payload {}",
                res.error(),
                res.error().message(),
                now,
                event.miner_address,
                common::hex_lower(event.callback_payload));

            OUTCOME_TRY(state.deleteClaim(event.miner_address));
          }
          return outcome::success();
        }));
        OUTCOME_TRY(state.cron_event_queue.remove(epoch));
      }
    }
    state.first_cron_epoch = now + 1;

    // Lotus gas conformance
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRYA(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(state.claims.hamt.loadRoot());
    OUTCOME_TRY(runtime.commitState(state));

    return outcome::success();
  }

  outcome::result<void> processBatchProofVerifiers(Runtime &runtime,
                                                   State &state) {
    if (state.proof_validation_batch.has_value()) {
      OUTCOME_TRY(
          verified,
          runtime.verifyBatchSeals(state.proof_validation_batch.value()));
      OUTCOME_TRY(miners, state.proof_validation_batch.value().keys());
      for (const auto &miner : miners) {
        // refuse to process proofs for miner with no claim
        OUTCOME_TRY(has_claim, state.claims.has(miner));
        if (!has_claim) {
          spdlog::warn("skipping batch verifies for unknown miner {}",
                       encodeToString(miner));
          break;
        }

        const auto seals_verified = verified.find(miner);
        if (seals_verified == verified.end()) {
          spdlog::warn("batch verify seals syscall implemented incorrectly");
          return VMExitCode::kErrNotFound;
        }
        std::vector<SectorNumber> successful;
        OUTCOME_TRY(verifies, state.proof_validation_batch.value().get(miner));
        OUTCOME_TRY(verifies.visit(
            [&](auto i, auto &seal_info) -> outcome::result<void> {
              auto sector{seal_info.sector.sector};
              if (seals_verified->second.at(i)
                  && (std::find(successful.begin(), successful.end(), sector)
                      == successful.end())) {
                successful.push_back(sector);
              }
              return outcome::success();
            }));

        // The non-fatal exit code is explicitly ignored
        const auto result = runtime.sendM<miner::ConfirmSectorProofsValid>(
            miner, {successful}, 0);
        if (result.has_error() && isFatal(result.error())) {
          return result.error();
        }
      }

      state.proof_validation_batch = boost::none;

      // Lotus gas conformance
      OUTCOME_TRYA(state, runtime.getCurrentActorStateCbor<State>());
      OUTCOME_TRY(runtime.commitState(state));
    }

    return outcome::success();
  }

  outcome::result<void> validateMinerHasClaim(Runtime &runtime,
                                              State &state,
                                              const Address &miner) {
    auto claims{state.claims};
    OUTCOME_TRY(runtime.requireNoError(claims.hamt.loadRoot(),
                                       VMExitCode::kErrIllegalState));
    OUTCOME_TRY(has,
                runtime.requireNoError(claims.has(miner),
                                       VMExitCode::kErrIllegalState));
    if (!has) {
      return runtime.abort(VMExitCode::kErrForbidden);
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kSystemActorAddress));
    OUTCOME_TRY(runtime.commitState(State::empty(runtime)));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(CreateMiner) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    OUTCOME_TRY(miner_params,
                encodeActorParams(miner::Construct::Params{
                    .owner = params.owner,
                    .worker = params.worker,
                    .control_addresses = {},
                    .seal_proof_type = params.seal_proof_type,
                    .peer_id = params.peer_id,
                    .multiaddresses = params.multiaddresses}));
    OUTCOME_TRY(addresses_created,
                runtime.sendM<init::Exec>(kInitAddress,
                                          {kStorageMinerCodeCid, miner_params},
                                          runtime.getValueReceived()));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(state.claims.set(addresses_created.id_address,
                                 {params.seal_proof_type, 0, 0}));
    ++state.miner_count;
    OUTCOME_TRY(runtime.commitState(state));
    return Result{addresses_created.id_address,
                  addresses_created.robust_address};
  }

  ACTOR_METHOD_IMPL(UpdateClaimedPower) {
    OUTCOME_TRY(runtime.validateImmediateCallerType(kStorageMinerCodeCid));
    const Address miner_address = runtime.getImmediateCaller();
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(state.addToClaim(
        miner_address, params.raw_byte_delta, params.quality_adjusted_delta));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(EnrollCronEvent) {
    OUTCOME_TRY(runtime.validateImmediateCallerType(kStorageMinerCodeCid));
    OUTCOME_TRY(runtime.validateArgument(params.event_epoch >= 0));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(
        state.appendCronEvent(params.event_epoch,
                              {.miner_address = runtime.getImmediateCaller(),
                               .callback_payload = params.payload}));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(OnEpochTickEnd) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kCronAddress));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    OUTCOME_TRY(processBatchProofVerifiers(runtime, state));
    OUTCOME_TRY(processDeferredCronEvents(runtime, state));

    // Lotus gas conformance
    OUTCOME_TRYA(state, runtime.getCurrentActorStateCbor<State>());

    const auto [raw_power, qa_power] = state.getCurrentTotalPower();
    state.this_epoch_pledge = state.total_pledge;
    state.this_epoch_raw_power = raw_power;
    state.this_epoch_qa_power = qa_power;

    state.updateSmoothedEstimate(1);

    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(runtime.sendM<reward::UpdateNetworkKPI>(
        kRewardAddress, state.this_epoch_raw_power, 0));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(UpdatePledgeTotal) {
    OUTCOME_TRY(runtime.validateImmediateCallerType(kStorageMinerCodeCid));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(
        validateMinerHasClaim(runtime, state, runtime.getImmediateCaller()));
    OUTCOME_TRY(state.addPledgeTotal(params));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(SubmitPoRepForBulkVerify) {
    OUTCOME_TRY(runtime.validateImmediateCallerType(kStorageMinerCodeCid));
    const auto miner{runtime.getImmediateCaller()};
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    OUTCOME_TRY(validateMinerHasClaim(runtime, state, miner));

    if (!state.proof_validation_batch.has_value()) {
      state.proof_validation_batch.emplace(runtime.getIpfsDatastore());
    }
    OUTCOME_TRY(found, state.proof_validation_batch->tryGet(miner));
    if (found.has_value()) {
      OUTCOME_TRY(size, found.value().size());
      if (size >= kMaxMinerProveCommitsPerEpoch) {
        return kErrTooManyProveCommits;
      }
    }
    OUTCOME_TRY(
        Multimap::append(state.proof_validation_batch.value(), miner, params));

    // Lotus gas conformance
    OUTCOME_TRY(state.proof_validation_batch->hamt.flush());

    OUTCOME_TRY(runtime.chargeGas(kGasOnSubmitVerifySeal));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(CurrentTotalPower) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    return Result{
        .raw_byte_power = state.this_epoch_raw_power,
        .quality_adj_power = state.this_epoch_qa_power,
        .pledge_collateral = state.this_epoch_pledge,
        .quality_adj_power_smoothed = state.this_epoch_qa_power_smoothed};
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
