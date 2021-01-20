/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor.hpp"
#include "vm/actor/builtin/v3/account/account_actor.hpp"
#include "vm/actor/builtin/v3/codes.hpp"
#include "vm/actor/builtin/v3/miner/miner_actor.hpp"
#include "vm/actor/builtin/v3/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v3/storage_power/storage_power_actor_export.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using v0::miner::CronEventPayload;
  using v0::miner::CronEventType;
  using v0::miner::kWPoStChallengeWindow;
  using v0::miner::kWPoStPeriodDeadlines;
  using v0::miner::resolveControlAddress;
  using v0::miner::SectorOnChainInfo;
  using v0::miner::VestingFunds;
  using v2::miner::checkControlAddresses;
  using v2::miner::checkPeerInfo;

  /**
   * Resolves address via v3 Account actor
   */
  outcome::result<Address> resolveWorkerAddress(Runtime &runtime,
                                                const Address &address) {
    OUTCOME_TRY(resolved, runtime.resolveAddress(address));
    VM_ASSERT(resolved.isId());
    const auto resolved_code = runtime.getActorCodeID(resolved);
    REQUIRE_NO_ERROR(resolved_code, VMExitCode::kErrIllegalArgument);
    OUTCOME_TRY(
        runtime.validateArgument(resolved_code.value() == kAccountCodeCid));

    if (!address.isBls()) {
      OUTCOME_TRY(pubkey_addres,
                  runtime.sendM<account::PubkeyAddress>(resolved, {}, 0));
      OUTCOME_TRY(runtime.validateArgument(pubkey_addres.isBls()));
    }

    return std::move(resolved);
  }

  /**
   * Registers first cron callback for epoch before the first proving period
   * starts.
   * Calls StoragePowerActor v3
   */
  outcome::result<void> enrollCronEvent(Runtime &runtime,
                                        ChainEpoch event_epoch,
                                        const CronEventPayload &payload) {
    OUTCOME_TRY(encoded_params, codec::cbor::encode(payload));
    OUTCOME_TRY(runtime.sendM<storage_power::EnrollCronEvent>(
        kStoragePowerAddress, {event_epoch, encoded_params}, 0));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));
    OUTCOME_TRY(checkControlAddresses(runtime, params.control_addresses));
    OUTCOME_TRY(checkPeerInfo(runtime, params.peer_id, params.multiaddresses));
    OUTCOME_TRY(owner, resolveControlAddress(runtime, params.owner));
    OUTCOME_TRY(worker, resolveWorkerAddress(runtime, params.worker));
    std::vector<Address> control_addresses;
    for (const auto &control_address : params.control_addresses) {
      OUTCOME_TRY(resolved, resolveControlAddress(runtime, control_address));
      control_addresses.push_back(resolved);
    }

    State state;

    // Lotus gas conformance - flush empty hamt
    state.precommitted_sectors = {};
    runtime.getIpfsDatastore()->load(state);
    OUTCOME_TRY(state.precommitted_sectors.hamt.flush());

    // Lotus gas conformance - flush empty hamt
    state.precommitted_setctors_expiry = {};
    runtime.getIpfsDatastore()->load(state);

    OUTCOME_TRY(empty_amt_cid, state.precommitted_setctors_expiry.amt.flush());

    RleBitset allocated_sectors;
    OUTCOME_TRY(state.allocated_sectors.set(allocated_sectors));

    OUTCOME_TRY(
        deadlines,
        Deadlines::makeEmpty(runtime.getIpfsDatastore(), empty_amt_cid));
    OUTCOME_TRY(state.deadlines.set(deadlines));

    VestingFunds vesting_funds;
    OUTCOME_TRY(state.vesting_funds.set(vesting_funds));

    const auto current_epoch = runtime.getCurrentEpoch();
    const auto offset =
        v0::miner::Construct::assignProvingPeriodOffset(runtime, current_epoch);
    REQUIRE_NO_ERROR(offset, VMExitCode::kErrSerialization);
    const auto period_start = v2::miner::Construct::currentProvingPeriodStart(
        current_epoch, offset.value());
    OUTCOME_TRY(runtime.requireState(period_start <= current_epoch));
    state.proving_period_start = period_start;

    OUTCOME_TRY(deadline_index,
                v2::miner::Construct::currentDeadlineIndex(current_epoch,
                                                           period_start));
    OUTCOME_TRY(runtime.requireState(deadline_index < kWPoStPeriodDeadlines));
    state.current_deadline = deadline_index;

    OUTCOME_TRY(miner_info,
                MinerInfo::make(owner,
                                worker,
                                control_addresses,
                                params.peer_id,
                                params.multiaddresses,
                                params.seal_proof_type));
    OUTCOME_TRY(state.info.set(miner_info));

    // construct with empty already cid stored in ipld to avoid gas charge
    state.sectors = adt::Array<SectorOnChainInfo>(empty_amt_cid,
                                                  runtime.getIpfsDatastore());

    OUTCOME_TRY(runtime.commitState(state));

    const ChainEpoch deadline_close =
        period_start + kWPoStChallengeWindow * (1 + deadline_index);
    OUTCOME_TRY(enrollCronEvent(
        runtime, deadline_close - 1, {CronEventType::kProvingDeadline}));

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(DisputeWindowedPoSt) {
    // TODO (a.chernyshov) implement
    return VMExitCode::kNotImplemented;
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<ControlAddresses>(),
      exportMethod<ChangeWorkerAddress>(),
      exportMethod<ChangePeerId>(),
      exportMethod<SubmitWindowedPoSt>(),
      exportMethod<PreCommitSector>(),
      exportMethod<ProveCommitSector>(),
      exportMethod<ExtendSectorExpiration>(),
      exportMethod<TerminateSectors>(),
      exportMethod<DeclareFaults>(),
      exportMethod<DeclareFaultsRecovered>(),
      exportMethod<OnDeferredCronEvent>(),
      exportMethod<CheckSectorProven>(),
      exportMethod<ApplyRewards>(),
      exportMethod<ReportConsensusFault>(),
      exportMethod<WithdrawBalance>(),
      exportMethod<ConfirmSectorProofsValid>(),
      exportMethod<ChangeMultiaddresses>(),
      exportMethod<CompactPartitions>(),
      exportMethod<CompactSectorNumbers>(),
      exportMethod<ConfirmUpdateWorkerKey>(),
      exportMethod<RepayDebt>(),
      exportMethod<ChangeOwnerAddress>(),
      exportMethod<DisputeWindowedPoSt>(),
  };

}  // namespace fc::vm::actor::builtin::v3::miner
