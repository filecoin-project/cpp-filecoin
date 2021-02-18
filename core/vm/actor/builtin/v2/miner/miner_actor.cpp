/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/miner/miner_actor.hpp"
#include "vm/actor/builtin/v2/account/account_actor.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v2/miner/policy.hpp"
#include "vm/actor/builtin/v2/miner/types.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_export.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::RleBitset;
  using primitives::sector::RegisteredSealProof;
  using toolchain::Toolchain;
  using v0::miner::CronEventPayload;
  using v0::miner::CronEventType;
  using v0::miner::Deadlines;
  using v0::miner::kWPoStChallengeWindow;
  using v0::miner::kWPoStPeriodDeadlines;
  using v0::miner::kWPoStProvingPeriod;
  using v0::miner::resolveControlAddress;
  using v0::miner::SectorOnChainInfo;
  using v0::miner::VestingFunds;

  /**
   * Resolves an address to an ID address and verifies that it is address of an
   * account actor with an associated BLS key. The worker must be BLS since the
   * worker key will be used alongside a BLS-VRF.
   * @param runtime
   * @param address to resolve
   * @return resolved address
   */
  outcome::result<Address> resolveWorkerAddress(Runtime &runtime,
                                                const Address &address) {
    const auto resolved = runtime.resolveAddress(address);
    OUTCOME_TRY(runtime.validateArgument(!resolved.has_error()));
    VM_ASSERT(resolved.value().isId());
    const auto resolved_code = runtime.getActorCodeID(resolved.value());
    OUTCOME_TRY(runtime.validateArgument(!resolved_code.has_error()));
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateArgument(
        resolved_code.value() == address_matcher->getAccountCodeId()));

    if (!address.isBls()) {
      const auto pubkey_addres =
          runtime.sendM<account::PubkeyAddress>(resolved.value(), {}, 0);
      OUTCOME_TRY(runtime.validateArgument(pubkey_addres.value().isBls()));
    }

    return std::move(resolved.value());
  }

  /**
   * Registers first cron callback for epoch before the first proving period
   * starts.
   */
  outcome::result<void> enrollCronEvent(Runtime &runtime,
                                        ChainEpoch event_epoch,
                                        const CronEventPayload &payload) {
    const auto encoded_params = codec::cbor::encode(payload);
    OUTCOME_TRY(runtime.validateArgument(!encoded_params.has_error()));
    REQUIRE_SUCCESS(runtime.sendM<storage_power::EnrollCronEvent>(
        kStoragePowerAddress, {event_epoch, encoded_params.value()}, 0));
    return outcome::success();
  }

  outcome::result<void> checkControlAddresses(
      const Runtime &runtime, const std::vector<Address> &control_addresses) {
    return runtime.validateArgument(control_addresses.size()
                                    <= kMaxControlAddresses);
  }

  outcome::result<void> checkPeerInfo(
      const Runtime &runtime,
      const Buffer &peer_id,
      const std::vector<Multiaddress> &multiaddresses) {
    OUTCOME_TRY(runtime.validateArgument(peer_id.size() <= kMaxPeerIDLength));
    size_t total_size = 0;
    for (const auto &multiaddress : multiaddresses) {
      OUTCOME_TRY(
          runtime.validateArgument(!multiaddress.getBytesAddress().empty()));
      total_size += multiaddress.getBytesAddress().size();
    }
    OUTCOME_TRY(runtime.validateArgument(total_size <= kMaxMultiaddressData));
    return outcome::success();
  }

  /**
   * Checks whether a seal proof type is supported for new miners and sectors.
   */
  outcome::result<void> canPreCommitSealProof(
      const Runtime &runtime,
      const RegisteredSealProof &seal_proof_type,
      const NetworkVersion &network_version) {
    if (network_version < NetworkVersion::kVersion7) {
      OUTCOME_TRY(runtime.validateArgument(
          kPreCommitSealProofTypesV0.find(seal_proof_type)
          != kPreCommitSealProofTypesV0.end()));
    } else if (network_version == NetworkVersion::kVersion7) {
      OUTCOME_TRY(runtime.validateArgument(
          kPreCommitSealProofTypesV7.find(seal_proof_type)
          != kPreCommitSealProofTypesV7.end()));
    } else if (network_version >= NetworkVersion::kVersion8) {
      OUTCOME_TRY(runtime.validateArgument(
          kPreCommitSealProofTypesV8.find(seal_proof_type)
          != kPreCommitSealProofTypesV8.end()));
    }
    return outcome::success();
  }

  outcome::result<uint64_t> Construct::currentDeadlineIndex(
      const Runtime &runtime,
      const ChainEpoch &current_epoch,
      const ChainEpoch &period_start) {
    VM_ASSERT(current_epoch >= period_start);
    return (current_epoch - period_start) / kWPoStChallengeWindow;
  }

  ChainEpoch Construct::currentProvingPeriodStart(ChainEpoch current_epoch,
                                                  ChainEpoch offset) {
    const auto current_modulus = current_epoch % kWPoStProvingPeriod;
    const ChainEpoch period_progress =
        current_modulus >= offset
            ? current_modulus - offset
            : kWPoStProvingPeriod - (offset - current_modulus);
    const ChainEpoch period_start = current_epoch - period_progress;
    return period_start;
  }

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));
    OUTCOME_TRY(checkControlAddresses(runtime, params.control_addresses));
    OUTCOME_TRY(checkPeerInfo(runtime, params.peer_id, params.multiaddresses));
    CHANGE_ERROR_ABORT(
        canPreCommitSealProof(
            runtime, params.seal_proof_type, runtime.getNetworkVersion()),
        VMExitCode::kErrIllegalArgument);
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
    REQUIRE_NO_ERROR_A(
        offset,
        v0::miner::Construct::assignProvingPeriodOffset(runtime, current_epoch),
        VMExitCode::kErrSerialization);
    const auto period_start = currentProvingPeriodStart(current_epoch, offset);
    VM_ASSERT(period_start <= current_epoch);
    state.proving_period_start = period_start;

    OUTCOME_TRY(deadline_index,
                currentDeadlineIndex(runtime, current_epoch, period_start));
    VM_ASSERT(deadline_index < kWPoStPeriodDeadlines);
    state.current_deadline = deadline_index;

    REQUIRE_NO_ERROR_A(miner_info,
                       MinerInfo::make(owner,
                                       worker,
                                       control_addresses,
                                       params.peer_id,
                                       params.multiaddresses,
                                       params.seal_proof_type),
                       VMExitCode::kErrIllegalArgument);
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

  ACTOR_METHOD_IMPL(ApplyRewards) {
    // TODO (a.chernyshov) FIL-310 - implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ConfirmUpdateWorkerKey) {
    // TODO (a.chernyshov) FIL-317 implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(RepayDebt) {
    // TODO (a.chernyshov) FIL-318 implement
    return VMExitCode::kNotImplemented;
  }

  ACTOR_METHOD_IMPL(ChangeOwnerAddress) {
    // TODO (a.chernyshov) FIL-319 implement
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
  };

}  // namespace fc::vm::actor::builtin::v2::miner
