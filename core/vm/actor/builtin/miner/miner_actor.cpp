/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/miner/miner_actor.hpp"

#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/actor/builtin/market/actor.hpp"
#include "vm/actor/builtin/miner/policy.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::miner {
  using primitives::kChainEpochUndefined;
  using primitives::address::Protocol;
  using primitives::sector::OnChainSealVerifyInfo;
  using primitives::sector::PoStCandidate;
  using primitives::sector::SectorInfo;
  using runtime::DomainSeparationTag;
  using storage_power::kWindowedPostChallengeDuration;
  using storage_power::kWindowedPostFailureLimit;
  using storage_power::SectorTerminationType;

  void loadState(Runtime &runtime, MinerActorState &state) {
    state.precommitted_sectors.load(runtime);
    state.sectors.load(runtime);
    state.proving_set.load(runtime);
  }

  outcome::result<MinerActorState> loadState(Runtime &runtime) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<MinerActorState>());
    loadState(runtime, state);
    return std::move(state);
  }

  outcome::result<void> commitState(Runtime &runtime, MinerActorState &state) {
    OUTCOME_TRY(state.precommitted_sectors.flush());
    OUTCOME_TRY(state.sectors.flush());
    OUTCOME_TRY(state.proving_set.flush());
    return runtime.commitState(state);
  }

  outcome::result<MinerActorState> assertCallerIsWorker(Runtime &runtime) {
    OUTCOME_TRY(state, loadState(runtime));
    if (runtime.getImmediateCaller() != state.info.worker) {
      return VMExitCode::MINER_ACTOR_WRONG_CALLER;
    }
    return std::move(state);
  }

  /**
   * Resolves an address to an ID address and verifies that it is address of an
   * account or multisig actor
   */
  outcome::result<Address> resolveOwnerAddress(Runtime &runtime,
                                               const Address &address) {
    OUTCOME_TRY(id, runtime.resolveAddress(address));
    OUTCOME_TRY(code, runtime.getActorCodeID(id));
    if (!isSignableActor(code)) {
      return VMExitCode::MINER_ACTOR_OWNER_NOT_SIGNABLE;
    }
    return std::move(id);
  }

  /**
   * Resolves an address to an ID address and verifies that it is address of an
   * account actor with an associated BLS key. The worker must be BLS since the
   * worker key will be used alongside a BLS-VRF
   */
  outcome::result<Address> resolveWorkerAddress(Runtime &runtime,
                                                const Address &address) {
    OUTCOME_TRY(id, runtime.resolveAddress(address));
    OUTCOME_TRY(code, runtime.getActorCodeID(id));
    if (code != kAccountCodeCid) {
      return VMExitCode::MINER_ACTOR_MINER_NOT_ACCOUNT;
    }
    if (address.getProtocol() != Protocol::BLS) {
      OUTCOME_TRY(key, runtime.sendM<account::PubkeyAddress>(id, {}, 0));
      if (key.getProtocol() != Protocol::BLS) {
        return VMExitCode::MINER_ACTOR_MINER_NOT_BLS;
      }
    }
    return std::move(id);
  }

  outcome::result<void> enrollCronEvent(Runtime &runtime,
                                        ChainEpoch event_epoch,
                                        const CronEventPayload &payload) {
    OUTCOME_TRY(payload2, codec::cbor::encode(payload));
    OUTCOME_TRY(runtime.sendM<storage_power::EnrollCronEvent>(
        kStoragePowerAddress,
        {
            .event_epoch = event_epoch,
            .payload = Buffer{payload2},
        },
        0));
    return outcome::success();
  }

  bool hasDuplicateTickets(const std::vector<PoStCandidate> &candidates) {
    std::set<int64_t> set;
    for (auto &candidate : candidates) {
      if (!set.insert(candidate.challenge_index).second) {
        return true;
      }
    }
    return false;
  }

  outcome::result<void> verifyWindowedPoSt(
      Runtime &runtime,
      MinerActorState &state,
      const OnChainPoStVerifyInfo &params) {
    if (hasDuplicateTickets(params.candidates)) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }
    if (params.candidates.size() != kNumWindowedPoStSectors) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }

    std::vector<SectorInfo> sectors;
    OUTCOME_TRY(state.proving_set.visit(
        [&](auto, auto &sector) -> outcome::result<void> {
          if (sector.declared_fault_epoch != kChainEpochUndefined
              || sector.declared_fault_duration != kChainEpochUndefined) {
            return VMExitCode::MINER_ACTOR_ILLEGAL_STATE;
          }
          if (state.fault_set.find(sector.info.sector)
              == state.fault_set.end()) {
            sectors.push_back({
                .registered_proof = sector.info.registered_proof,
                .sector = sector.info.sector,
                .sealed_cid = sector.info.sealed_cid,
            });
          }
          return outcome::success();
        }));

    auto challenge_count = (sectors.size() * kWindowedPoStSampleRateNumer)
                               / kWindowedPoStSampleRateDenum
                           + 1;

    OUTCOME_TRY(miner, runtime.resolveAddress(runtime.getCurrentReceiver()));
    OUTCOME_TRY(seed, codec::cbor::encode(miner));
    OUTCOME_TRY(verified,
                runtime.verifyPoSt({
                    .randomness = runtime.getRandomness(
                        DomainSeparationTag::PoStDST,
                        state.post_state.proving_period_start,
                        Buffer{seed}),
                    .candidates = params.candidates,
                    .proofs = params.proofs,
                    .eligible_sectors = sectors,
                    .prover = miner.getId(),
                    .challenge_count = challenge_count,
                }));
    if (!verified) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }
    return outcome::success();
  }

  outcome::result<void> confirmPaymentAndRefundChange(Runtime &runtime,
                                                      TokenAmount expected) {
    TokenAmount extra = expected - runtime.getValueReceived();
    if (extra < 0) {
      return VMExitCode::MINER_ACTOR_INSUFFICIENT_FUNDS;
    }
    if (extra > 0) {
      OUTCOME_TRY(runtime.sendFunds(runtime.getImmediateCaller(), extra));
    }
    return outcome::success();
  }

  bool inChallengeWindow(const MinerActorState &state, Runtime &runtime) {
    return runtime.getCurrentEpoch() > state.post_state.proving_period_start;
  }

  outcome::result<EpochDuration> maxSealDuration(RegisteredProof type) {
    switch (type) {
      case RegisteredProof::StackedDRG32GiBSeal:
      case RegisteredProof::StackedDRG2KiBSeal:
      case RegisteredProof::StackedDRG8MiBSeal:
      case RegisteredProof::StackedDRG512MiBSeal:
        return 10000;
      default:
        break;
    }
    return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
  }

  outcome::result<void> verifySeal(Runtime &runtime,
                                   const OnChainSealVerifyInfo &info) {
    ChainEpoch current_epoch = runtime.getCurrentEpoch();
    if (current_epoch <= info.interactive_epoch) {
      return VMExitCode::MINER_ACTOR_WRONG_EPOCH;
    }

    OUTCOME_TRY(duration, maxSealDuration(info.registered_proof));
    if (info.seal_rand_epoch < current_epoch - kChainFinalityish - duration) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }

    OUTCOME_TRY(comm_d,
                runtime.sendM<market::ComputeDataCommitment>(
                    kStorageMarketAddress,
                    {
                        .deals = info.deals,
                        .sector_type = info.registered_proof,
                    },
                    0));

    OUTCOME_TRY(miner, runtime.resolveAddress(runtime.getCurrentReceiver()));
    OUTCOME_TRY(runtime.verifySeal({
        .sector =
            {
                .miner = miner.getId(),
                .sector = info.sector,
            },
        .info = info,
        .randomness = runtime.getRandomness(DomainSeparationTag::SealRandomness,
                                            info.seal_rand_epoch),
        .interactive_randomness = runtime.getRandomness(
            DomainSeparationTag::InteractiveSealChallengeSeed,
            info.interactive_epoch),
        .unsealed_cid = comm_d,
    }));
    return outcome::success();
  }

  SectorStorageWeightDesc asStorageWeightDesc(SectorSize sector_size,
                                              const SectorOnChainInfo &sector) {
    return {
        .sector_size = sector_size,
        .duration = sector.info.expiration - sector.activation_epoch,
        .deal_weight = sector.deal_weight,
    };
  }

  outcome::result<void> requestTerminatePower(
      Runtime &runtime,
      SectorTerminationType type,
      const std::vector<SectorStorageWeightDesc> &weights,
      TokenAmount pledge) {
    OUTCOME_TRY(runtime.sendM<storage_power::OnSectorTerminate>(
        kStoragePowerAddress,
        {
            .termination_type = type,
            .weights = weights,
            .pledge = pledge,
        },
        0));
    return outcome::success();
  }

  outcome::result<void> requestTerminateDeals(
      Runtime &runtime, const std::vector<DealId> &deals) {
    OUTCOME_TRY(
        runtime.sendM<market::OnMinerSectorsTerminate>(kStorageMarketAddress,
                                                       {
                                                           .deals = deals,
                                                       },
                                                       0));
    return outcome::success();
  }

  outcome::result<void> requestBeginFaults(
      Runtime &runtime,
      const std::vector<SectorStorageWeightDesc> &weights,
      TokenAmount pledge) {
    OUTCOME_TRY(
        runtime.sendM<storage_power::OnSectorTemporaryFaultEffectiveBegin>(
            kStoragePowerAddress,
            {
                .weights = weights,
                .pledge = pledge,
            },
            0));
    return outcome::success();
  }

  outcome::result<void> requestEndFaults(
      Runtime &runtime,
      const std::vector<SectorStorageWeightDesc> &weights,
      TokenAmount pledge) {
    OUTCOME_TRY(
        runtime.sendM<storage_power::OnSectorTemporaryFaultEffectiveEnd>(
            kStoragePowerAddress,
            {
                .weights = weights,
                .pledge = pledge,
            },
            0));
    return outcome::success();
  }

  outcome::result<void> terminateSectorsInternal(Runtime &runtime,
                                                 MinerActorState &state,
                                                 const RleBitset &sectors,
                                                 SectorTerminationType type) {
    std::vector<DealId> deals;
    std::vector<SectorStorageWeightDesc> all_weights, fault_weights;
    TokenAmount all_pledges{0};
    TokenAmount fault_pledges{0};
    for (auto sector_num : sectors) {
      OUTCOME_TRY(found, state.sectors.has(sector_num));
      if (!found) {
        return VMExitCode::MINER_ACTOR_NOT_FOUND;
      }
      OUTCOME_TRY(sector, state.sectors.get(sector_num));
      deals.insert(deals.end(),
                   sector.info.deal_ids.begin(),
                   sector.info.deal_ids.end());
      all_pledges += sector.pledge_requirement;
      auto weight = asStorageWeightDesc(state.info.sector_size, sector);
      all_weights.push_back(weight);
      if (state.fault_set.find(sector_num) != state.fault_set.end()) {
        fault_weights.push_back(weight);
        fault_pledges += sector.pledge_requirement;
      }
      OUTCOME_TRY(state.sectors.remove(sector_num));
    }
    if (!inChallengeWindow(state, runtime)) {
      state.proving_set = state.sectors;
    }
    OUTCOME_TRY(commitState(runtime, state));
    if (!fault_weights.empty()) {
      OUTCOME_TRY(requestEndFaults(runtime, fault_weights, fault_pledges));
    }
    OUTCOME_TRY(requestTerminateDeals(runtime, deals));
    OUTCOME_TRY(requestTerminatePower(runtime, type, all_weights, all_pledges));
    return outcome::success();
  }

  outcome::result<void> checkTemporaryFaultEvents(
      Runtime &runtime,
      MinerActorState &state,
      const boost::optional<RleBitset> &sectors) {
    if (!sectors) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }
    std::vector<SectorStorageWeightDesc> begin_weights, end_weights;
    TokenAmount begin_pledges{0};
    TokenAmount end_pledges{0};
    for (auto sector_num : *sectors) {
      OUTCOME_TRY(found, state.sectors.has(sector_num));
      if (!found) {
        continue;  // Sector has been terminated
      }
      OUTCOME_TRY(sector, state.sectors.get(sector_num));
      if (state.fault_set.find(sector_num) == state.fault_set.end()) {
        if (runtime.getCurrentEpoch() >= sector.declared_fault_epoch) {
          begin_pledges += sector.pledge_requirement;
          begin_weights.push_back(
              asStorageWeightDesc(state.info.sector_size, sector));
          state.fault_set.insert(sector_num);
        }
      } else {
        if (runtime.getCurrentEpoch()
            >= sector.declared_fault_epoch + sector.declared_fault_duration) {
          sector.declared_fault_epoch = kChainEpochUndefined;
          sector.declared_fault_duration = kChainEpochUndefined;
          end_pledges += sector.pledge_requirement;
          end_weights.push_back(
              asStorageWeightDesc(state.info.sector_size, sector));
          state.fault_set.erase(sector_num);
          OUTCOME_TRY(state.sectors.set(sector_num, sector));
        }
      }
    }
    OUTCOME_TRY(commitState(runtime, state));
    if (!begin_weights.empty()) {
      OUTCOME_TRY(requestBeginFaults(runtime, begin_weights, begin_pledges));
    }
    if (!end_weights.empty()) {
      OUTCOME_TRY(requestEndFaults(runtime, end_weights, end_pledges));
    }
    return outcome::success();
  }

  outcome::result<void> checkPrecommitExpiry(
      Runtime &runtime,
      MinerActorState &state,
      const boost::optional<RleBitset> &sectors,
      RegisteredProof registered_proof) {
    if (!sectors) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }
    OUTCOME_TRY(duration, maxSealDuration(registered_proof));
    TokenAmount to_burn;
    for (auto sector_num : *sectors) {
      OUTCOME_TRY(found, state.precommitted_sectors.has(sector_num));
      if (!found) {
        break;
      }
      OUTCOME_TRY(precommit, state.precommitted_sectors.get(sector_num));
      if (runtime.getCurrentEpoch() - precommit.precommit_epoch <= duration) {
        break;
      }
      OUTCOME_TRY(state.precommitted_sectors.remove(sector_num));
      to_burn += precommit.precommit_deposit;
    }
    OUTCOME_TRY(commitState(runtime, state));
    OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, to_burn));
    return outcome::success();
  }

  outcome::result<void> checkSectorExpiry(
      Runtime &runtime,
      MinerActorState &state,
      const boost::optional<RleBitset> &sectors) {
    if (!sectors) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }
    RleBitset to_terminate;
    for (auto sector_num : *sectors) {
      OUTCOME_TRY(found, state.sectors.has(sector_num));
      if (!found) {
        continue;
      }
      OUTCOME_TRY(sector, state.sectors.get(sector_num));
      if (runtime.getCurrentEpoch() >= sector.info.expiration) {
        to_terminate.insert(sector_num);
      }
    }
    return terminateSectorsInternal(
        runtime,
        state,
        to_terminate,
        SectorTerminationType::SECTOR_TERMINATION_EXPIRED);
  }

  outcome::result<void> checkPoStProvingPeriodExpiration(
      Runtime &runtime, MinerActorState &state) {
    if (runtime.getCurrentEpoch() < state.post_state.proving_period_start
                                        + kWindowedPostChallengeDuration) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_STATE;
    }
    ++state.post_state.num_consecutive_failures;
    state.post_state.proving_period_start += kProvingPeriod;
    OUTCOME_TRY(commitState(runtime, state));
    if (state.post_state.num_consecutive_failures > kWindowedPostFailureLimit) {
      std::vector<DealId> deals;
      OUTCOME_TRY(
          state.sectors.visit([&](auto, auto &sector) -> outcome::result<void> {
            deals.insert(deals.end(),
                         sector.info.deal_ids.begin(),
                         sector.info.deal_ids.end());
            return outcome::success();
          }));
      OUTCOME_TRY(requestTerminateDeals(runtime, deals));
    }
    OUTCOME_TRY(runtime.sendM<storage_power::OnMinerWindowedPoStFailure>(
        kStoragePowerAddress,
        {
            .num_consecutive_failures =
                state.post_state.num_consecutive_failures,
        },
        0));
    return outcome::success();
  }

  outcome::result<void> commitWorkerKeyChange(Runtime &runtime,
                                              MinerActorState &state) {
    if (!state.info.pending_worker_key) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_STATE;
    }
    if (state.info.pending_worker_key->effective_at
        > runtime.getCurrentEpoch()) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_STATE;
    }
    state.info.worker = state.info.pending_worker_key->new_worker;
    state.info.pending_worker_key = boost::none;
    return commitState(runtime, state);
  }

  ACTOR_METHOD_IMPL(Construct) {
    if (runtime.getImmediateCaller() != kInitAddress) {
      return VMExitCode::MINER_ACTOR_WRONG_CALLER;
    }
    OUTCOME_TRY(owner, resolveOwnerAddress(runtime, params.owner));
    OUTCOME_TRY(worker, resolveWorkerAddress(runtime, params.worker));
    MinerActorState state{
        .precommitted_sectors = {},
        .sectors = {},
        .fault_set = {},
        .proving_set = {},
        .info =
            {
                .owner = owner,
                .worker = worker,
                .pending_worker_key = boost::none,
                .peer_id = params.peer_id,
                .sector_size = params.sector_size,
            },
        .post_state =
            {
                .proving_period_start = kChainEpochUndefined,
                .num_consecutive_failures = 0,
            },
    };
    loadState(runtime, state);
    OUTCOME_TRY(commitState(runtime, state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ControlAddresses) {
    OUTCOME_TRY(state, loadState(runtime));
    return Result{state.info.owner, state.info.worker};
  }

  ACTOR_METHOD_IMPL(ChangeWorkerAddress) {
    OUTCOME_TRY(state, loadState(runtime));
    if (runtime.getImmediateCaller() != state.info.owner) {
      return VMExitCode::MINER_ACTOR_WRONG_CALLER;
    }
    OUTCOME_TRY(worker, resolveWorkerAddress(runtime, params.new_worker));
    auto effective_at = runtime.getCurrentEpoch() + kWorkerKeyChangeDelay;
    state.info.pending_worker_key = WorkerKeyChange{
        .new_worker = worker,
        .effective_at = effective_at,
    };
    OUTCOME_TRY(commitState(runtime, state));
    OUTCOME_TRY(
        enrollCronEvent(runtime,
                        effective_at,
                        {
                            .event_type = CronEventType::WorkerKeyChange,
                            .sectors = boost::none,
                        }));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ChangePeerId) {
    OUTCOME_TRY(state, loadState(runtime));
    state.info.peer_id = params.new_id;
    OUTCOME_TRY(commitState(runtime, state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(SubmitWindowedPoSt) {
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    if (runtime.getCurrentEpoch() > state.post_state.proving_period_start
                                        + kWindowedPostChallengeDuration) {
      return VMExitCode::MINER_ACTOR_POST_TOO_LATE;
    }
    if (runtime.getCurrentEpoch() <= state.post_state.proving_period_start) {
      return VMExitCode::MINER_ACTOR_POST_TOO_EARLY;
    }
    OUTCOME_TRY(verifyWindowedPoSt(runtime, state, params));
    state.post_state.num_consecutive_failures = 0;
    state.post_state.proving_period_start += kProvingPeriod;
    state.proving_set = state.sectors;
    OUTCOME_TRY(commitState(runtime, state));
    OUTCOME_TRY(runtime.sendM<storage_power::OnMinerWindowedPoStSuccess>(
        kStoragePowerAddress, {}, 0));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(OnDeleteMiner) {
    if (runtime.getImmediateCaller() != kStoragePowerAddress) {
      return VMExitCode::MINER_ACTOR_WRONG_CALLER;
    }
    OUTCOME_TRY(runtime.deleteActor());
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(PreCommitSector) {
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));

    OUTCOME_TRY(already_commited, state.sectors.has(params.sector));
    if (already_commited) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }

    auto deposit = precommitDeposit(
        state.info.sector_size, params.expiration - runtime.getCurrentEpoch());
    OUTCOME_TRY(confirmPaymentAndRefundChange(runtime, deposit));

    OUTCOME_TRY(state.precommitted_sectors.set(
        params.sector,
        SectorPreCommitOnChainInfo{
            .info = params,
            .precommit_deposit = deposit,
            .precommit_epoch = runtime.getCurrentEpoch(),
        }));
    OUTCOME_TRY(commitState(runtime, state));

    if (params.expiration <= runtime.getCurrentEpoch()) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }

    OUTCOME_TRY(duration, maxSealDuration(params.registered_proof));
    OUTCOME_TRY(
        enrollCronEvent(runtime,
                        runtime.getCurrentEpoch() + duration + 1,
                        {
                            .event_type = CronEventType::PreCommitExpiry,
                            .sectors = RleBitset{params.sector},
                            .registered_proof = params.registered_proof,
                        }));

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ProveCommitSector) {
    OUTCOME_TRY(state, loadState(runtime));
    auto sector = params.sector;

    OUTCOME_TRY(precommit, state.precommitted_sectors.get(sector));

    OUTCOME_TRY(
        verifySeal(runtime,
                   {
                       .sealed_cid = precommit.info.sealed_cid,
                       .interactive_epoch =
                           precommit.precommit_epoch + kPreCommitChallengeDelay,
                       .registered_proof = precommit.info.registered_proof,
                       .proof = params.proof,
                       .deals = precommit.info.deal_ids,
                       .sector = precommit.info.sector,
                       .seal_rand_epoch = precommit.info.seal_epoch,
                   }));

    OUTCOME_TRY(deal_weight,
                runtime.sendM<market::VerifyDealsOnSectorProveCommit>(
                    kStorageMarketAddress,
                    {
                        .deals = precommit.info.deal_ids,
                        .sector_expiry = precommit.info.expiration,
                    },
                    0));

    OUTCOME_TRY(pledge,
                runtime.sendM<storage_power::OnSectorProveCommit>(
                    kStoragePowerAddress,
                    {
                        .weight =
                            SectorStorageWeightDesc{
                                .sector_size = state.info.sector_size,
                                .duration = precommit.info.expiration
                                            - runtime.getCurrentEpoch(),
                                .deal_weight = deal_weight,
                            },
                    },
                    0));

    OUTCOME_TRY(sectors, state.sectors.amt.count());
    OUTCOME_TRY(
        state.sectors.set(precommit.info.sector,
                          SectorOnChainInfo{
                              .info = precommit.info,
                              .activation_epoch = runtime.getCurrentEpoch(),
                              .deal_weight = deal_weight,
                              .pledge_requirement = pledge,
                              .declared_fault_epoch = {},
                              .declared_fault_duration = {},
                          }));

    OUTCOME_TRY(state.precommitted_sectors.remove(sector));

    if (sectors == 1) {
      state.post_state.proving_period_start =
          runtime.getCurrentEpoch() + kProvingPeriod;
    }

    if (!inChallengeWindow(state, runtime)) {
      state.proving_set = state.sectors;
    }

    OUTCOME_TRY(commitState(runtime, state));

    OUTCOME_TRY(enrollCronEvent(runtime,
                                precommit.info.expiration,
                                {
                                    .event_type = CronEventType::SectorExpiry,
                                    .sectors = RleBitset{sector},
                                }));

    if (sectors == 1) {
      OUTCOME_TRY(enrollCronEvent(
          runtime,
          state.post_state.proving_period_start
              + kWindowedPostChallengeDuration,
          {
              .event_type = CronEventType::WindowedPoStExpiration,
              .sectors = boost::none,
          }));
    }

    OUTCOME_TRY(
        runtime.sendFunds(state.info.worker, precommit.precommit_deposit));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ExtendSectorExpiration) {
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));

    OUTCOME_TRY(sector, state.sectors.get(params.sector));

    auto prev_weight{asStorageWeightDesc(state.info.sector_size, sector)};
    auto extension = params.new_expiration - sector.info.expiration;
    if (extension < 0) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }
    auto new_weight{prev_weight};
    new_weight.duration = prev_weight.duration + extension;

    OUTCOME_TRY(new_pledge,
                runtime.sendM<storage_power::OnSectorModifyWeightDesc>(
                    kStoragePowerAddress,
                    {
                        .prev_weight = prev_weight,
                        .prev_pledge = sector.pledge_requirement,
                        .new_weight = new_weight,
                    },
                    0));

    sector.info.expiration = params.new_expiration;
    sector.pledge_requirement = new_pledge;
    OUTCOME_TRY(state.sectors.set(sector.info.sector, sector));

    OUTCOME_TRY(commitState(runtime, state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(TerminateSectors) {
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    if (!params.sectors) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }
    OUTCOME_TRY(terminateSectorsInternal(
        runtime,
        state,
        *params.sectors,
        SectorTerminationType::SECTOR_TERMINATION_MANUAL));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(DeclareTemporaryFaults) {
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    if (params.duration <= 0) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }

    auto effective_at =
        runtime.getCurrentEpoch() + kDeclaredFaultEffectiveDelay;
    std::vector<SectorStorageWeightDesc> weights;
    for (auto sector_num : params.sectors) {
      if (state.fault_set.find(sector_num) != state.fault_set.end()) {
        continue;
      }
      OUTCOME_TRY(sector, state.sectors.get(sector_num));
      weights.push_back(asStorageWeightDesc(state.info.sector_size, sector));
      sector.declared_fault_epoch = effective_at;
      sector.declared_fault_duration = params.duration;
      OUTCOME_TRY(state.sectors.set(sector_num, sector));
    }
    OUTCOME_TRY(commitState(runtime, state));
    auto required_fee = temporaryFaultFee(weights, params.duration);

    OUTCOME_TRY(confirmPaymentAndRefundChange(runtime, required_fee));
    OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, required_fee));

    CronEventPayload cron{
        .event_type = CronEventType::TempFault,
        .sectors = params.sectors,
    };
    OUTCOME_TRY(enrollCronEvent(runtime, effective_at, cron));
    OUTCOME_TRY(enrollCronEvent(runtime, effective_at + params.duration, cron));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(OnDeferredCronEvent) {
    if (runtime.getImmediateCaller() != kStoragePowerAddress) {
      return VMExitCode::MINER_ACTOR_WRONG_CALLER;
    }
    OUTCOME_TRY(payload,
                codec::cbor::decode<CronEventPayload>(params.callback_payload));
    OUTCOME_TRY(state, loadState(runtime));
    switch (payload.event_type) {
      case CronEventType::TempFault: {
        OUTCOME_TRY(checkTemporaryFaultEvents(runtime, state, payload.sectors));
        break;
      }
      case CronEventType::PreCommitExpiry: {
        OUTCOME_TRY(checkPrecommitExpiry(
            runtime, state, payload.sectors, payload.registered_proof));
        break;
      }
      case CronEventType::SectorExpiry: {
        OUTCOME_TRY(checkSectorExpiry(runtime, state, payload.sectors));
        break;
      }
      case CronEventType::WindowedPoStExpiration: {
        OUTCOME_TRY(checkPoStProvingPeriodExpiration(runtime, state));
        break;
      }
      case CronEventType::WorkerKeyChange: {
        OUTCOME_TRY(commitWorkerKeyChange(runtime, state));
        break;
      }
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(CheckSectorProven) {
    OUTCOME_TRY(state, loadState(runtime));
    OUTCOME_TRY(found, state.sectors.has(params.sector));
    if (!found) {
      return VMExitCode::MINER_ACTOR_NOT_FOUND;
    }
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<ControlAddresses>(),
      exportMethod<ChangeWorkerAddress>(),
      exportMethod<ChangePeerId>(),
      exportMethod<SubmitWindowedPoSt>(),
      exportMethod<OnDeleteMiner>(),
      exportMethod<PreCommitSector>(),
      exportMethod<ProveCommitSector>(),
      exportMethod<ExtendSectorExpiration>(),
      exportMethod<TerminateSectors>(),
      exportMethod<DeclareTemporaryFaults>(),
      exportMethod<OnDeferredCronEvent>(),
      exportMethod<CheckSectorProven>(),
  };
}  // namespace fc::vm::actor::builtin::miner
