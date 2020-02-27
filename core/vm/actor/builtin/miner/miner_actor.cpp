/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/miner/miner_actor.hpp"

#include "primitives/chain_epoch/chain_epoch_codec.hpp"
#include "storage/amt/amt.hpp"
#include "storage/hamt/hamt.hpp"
#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/actor/builtin/market/actor.hpp"
#include "vm/actor/builtin/miner/policy.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::miner {
  using market::ComputeDataCommitmentParams;
  using market::kComputeDataCommitmentMethodNumber;
  using market::kOnMinerSectorsTerminateMethodNumber;
  using market::kVerifyDealsOnSectorProveCommitMethodNumber;
  using market::OnMinerSectorsTerminateParams;
  using market::VerifyDealsOnSectorProveCommitParams;
  using primitives::kChainEpochUndefined;
  using primitives::address::Protocol;
  using primitives::chain_epoch::uvarintKey;
  using primitives::sector::OnChainSealVerifyInfo;
  using primitives::sector::PoStCandidate;
  using primitives::sector::SectorInfo;
  using runtime::DomainSeparationTag;
  using storage::amt::Amt;
  using storage::hamt::Hamt;
  using storage_power::EnrollCronEventParams;
  using storage_power::kEnrollCronEventMethodNumber;
  using storage_power::kOnMinerSurprisePoStFailureMethodNumber;
  using storage_power::kOnMinerSurprisePoStSuccessMethodNumber;
  using storage_power::kOnSectorModifyWeightDescMethodNumber;
  using storage_power::kOnSectorProveCommitMethodNumber;
  using storage_power::kOnSectorTemporaryFaultEffectiveBeginMethodNumber;
  using storage_power::kOnSectorTemporaryFaultEffectiveEndMethodNumber;
  using storage_power::kOnSectorTerminateMethodNumber;
  using storage_power::OnMinerWindowedPoStFailureParams;
  using storage_power::OnSectorModifyWeightDescParams;
  using storage_power::OnSectorProveCommitParameters;
  using storage_power::OnSectorTemporaryFaultEffectiveBeginParameters;
  using storage_power::OnSectorTemporaryFaultEffectiveEndParameters;
  using storage_power::OnSectorTerminateParameters;
  using storage_power::SectorStorageWeightDesc;
  using storage_power::SectorTerminationType;

  outcome::result<MinerActorState> assertCallerIsWorker(Runtime &runtime) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<MinerActorState>());
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
      OUTCOME_TRY(key,
                  runtime.sendR<Address>(
                      id, account::kPubkeyAddressMethodNumber, {}, 0));
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
    OUTCOME_TRY(runtime.sendP(kStoragePowerAddress,
                              kEnrollCronEventMethodNumber,
                              EnrollCronEventParams{
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
      const MinerActorState &state,
      const SubmitWindowedPoStParams &params) {
    if (hasDuplicateTickets(params.candidates)) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }
    if (params.candidates.size() != kNumWindowedPoStSectors) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }

    std::vector<SectorInfo> sectors;
    OUTCOME_TRY(
        Amt(runtime.getIpfsDatastore(), state.proving_set)
            .visit([&](auto, auto &raw) -> outcome::result<void> {
              OUTCOME_TRY(sector, codec::cbor::decode<SectorOnChainInfo>(raw));
              if (sector.declared_fault_epoch != kChainEpochUndefined
                  || sector.declared_fault_duration != kChainEpochUndefined) {
                return VMExitCode::MINER_ACTOR_ILLEGAL_STATE;
              }
              if (state.fault_set.find(sector.info.sector)
                  == state.fault_set.end()) {
                sectors.push_back({
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
    OUTCOME_TRY(
        verified,
        runtime.verifyPoSt(state.info.sector_size,
                           {
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
      case RegisteredProof::WinStackedDRG32GiBSeal:
        return 10000;
      default:
        break;
    }
    return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
  }

  outcome::result<void> verifySeal(Runtime &runtime,
                                   SectorSize sector_size,
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
                runtime.sendPR<CID>(kStorageMarketAddress,
                                    kComputeDataCommitmentMethodNumber,
                                    ComputeDataCommitmentParams{
                                        .deals = info.deals,
                                        .sector_size = sector_size,
                                    },
                                    0));

    OUTCOME_TRY(miner, runtime.resolveAddress(runtime.getCurrentReceiver()));
    OUTCOME_TRY(runtime.verifySeal(
        sector_size,
        {
            .sector =
                {
                    .miner = miner.getId(),
                    .sector = info.sector,
                },
            .info = info,
            .randomness = runtime.getRandomness(
                DomainSeparationTag::SealRandomness, info.seal_rand_epoch),
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
    OUTCOME_TRY(runtime.sendP(kStoragePowerAddress,
                              kOnSectorTerminateMethodNumber,
                              OnSectorTerminateParameters{
                                  .termination_type = type,
                                  .weights = weights,
                                  .pledge = pledge,
                              },
                              0));
    return outcome::success();
  }

  outcome::result<void> requestTerminateDeals(
      Runtime &runtime, const std::vector<DealId> &deals) {
    OUTCOME_TRY(runtime.sendP(kStorageMarketAddress,
                              kOnMinerSectorsTerminateMethodNumber,
                              OnMinerSectorsTerminateParams{
                                  .deals = deals,
                              },
                              0));
    return outcome::success();
  }

  outcome::result<void> requestBeginFaults(
      Runtime &runtime,
      const std::vector<SectorStorageWeightDesc> &weights,
      TokenAmount pledge) {
    OUTCOME_TRY(runtime.sendP(kStoragePowerAddress,
                              kOnSectorTemporaryFaultEffectiveBeginMethodNumber,
                              OnSectorTemporaryFaultEffectiveBeginParameters{
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
    OUTCOME_TRY(runtime.sendP(kStoragePowerAddress,
                              kOnSectorTemporaryFaultEffectiveEndMethodNumber,
                              OnSectorTemporaryFaultEffectiveEndParameters{
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
    Amt amt_sectors{runtime.getIpfsDatastore(), state.sectors};
    std::vector<DealId> deals;
    std::vector<SectorStorageWeightDesc> all_weights, fault_weights;
    TokenAmount all_pledges{0};
    TokenAmount fault_pledges{0};
    for (auto sector_num : sectors) {
      OUTCOME_TRY(found, amt_sectors.contains(sector_num));
      if (!found) {
        return VMExitCode::MINER_ACTOR_NOT_FOUND;
      }
      OUTCOME_TRY(sector, amt_sectors.getCbor<SectorOnChainInfo>(sector_num));
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
      OUTCOME_TRY(amt_sectors.remove(sector_num));
    }
    if (!inChallengeWindow(state, runtime)) {
      state.proving_set = state.sectors;
    }
    OUTCOME_TRY(amt_sectors.flush());
    state.sectors = amt_sectors.cid();
    OUTCOME_TRY(runtime.commitState(state));
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
    Amt amt_sectors{runtime.getIpfsDatastore(), state.sectors};
    std::vector<SectorStorageWeightDesc> begin_weights, end_weights;
    TokenAmount begin_pledges{0};
    TokenAmount end_pledges{0};
    for (auto sector_num : *sectors) {
      OUTCOME_TRY(found, amt_sectors.contains(sector_num));
      if (!found) {
        continue;  // Sector has been terminated
      }
      OUTCOME_TRY(sector, amt_sectors.getCbor<SectorOnChainInfo>(sector_num));
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
          OUTCOME_TRY(amt_sectors.setCbor(sector_num, sector));
        }
      }
    }
    OUTCOME_TRY(amt_sectors.flush());
    state.sectors = amt_sectors.cid();
    OUTCOME_TRY(runtime.commitState(state));
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
    Hamt hamt_precommit{runtime.getIpfsDatastore(), state.precommitted_sectors};
    TokenAmount to_burn;
    for (auto sector_num : *sectors) {
      auto key = uvarintKey(sector_num);
      OUTCOME_TRY(found, hamt_precommit.contains(key));
      if (!found) {
        continue;
      }
      OUTCOME_TRY(precommit,
                  hamt_precommit.getCbor<SectorPreCommitOnChainInfo>(key));
      if (runtime.getCurrentEpoch() - precommit.precommit_epoch <= duration) {
        continue;
      }
      OUTCOME_TRY(hamt_precommit.remove(key));
      to_burn += precommit.precommit_deposit;
    }
    OUTCOME_TRY(hamt_precommit.flush());
    state.precommitted_sectors = hamt_precommit.cid();
    OUTCOME_TRY(runtime.commitState(state));
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
    Amt amt_sectors{runtime.getIpfsDatastore(), state.sectors};
    RleBitset to_terminate;
    for (auto sector_num : *sectors) {
      OUTCOME_TRY(found, amt_sectors.contains(sector_num));
      if (!found) {
        continue;
      }
      OUTCOME_TRY(sector, amt_sectors.getCbor<SectorOnChainInfo>(sector_num));
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
    OUTCOME_TRY(runtime.commitState(state));
    if (state.post_state.num_consecutive_failures > kWindowedPostFailureLimit) {
      std::vector<DealId> deals;
      OUTCOME_TRY(
          Amt(runtime.getIpfsDatastore(), state.sectors)
              .visit([&](auto, auto &sector_raw) -> outcome::result<void> {
                OUTCOME_TRY(sector,
                            codec::cbor::decode<SectorOnChainInfo>(sector_raw));
                deals.insert(deals.end(),
                             sector.info.deal_ids.begin(),
                             sector.info.deal_ids.end());
                return outcome::success();
              }));
      OUTCOME_TRY(requestTerminateDeals(runtime, deals));
    }
    OUTCOME_TRY(runtime.sendP(kStoragePowerAddress,
                              kOnMinerSurprisePoStFailureMethodNumber,
                              OnMinerWindowedPoStFailureParams{
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
    return runtime.commitState(state);
  }

  ACTOR_METHOD(constructor) {
    if (runtime.getImmediateCaller() != kInitAddress) {
      return VMExitCode::MINER_ACTOR_WRONG_CALLER;
    }
    OUTCOME_TRY(params2, decodeActorParams<ConstructorParams>(params));
    auto ipld = runtime.getIpfsDatastore();
    OUTCOME_TRY(empty_amt, Amt{ipld}.flush());
    OUTCOME_TRY(empty_hamt, Hamt{ipld}.flush());
    OUTCOME_TRY(owner, resolveOwnerAddress(runtime, params2.owner));
    OUTCOME_TRY(worker, resolveWorkerAddress(runtime, params2.worker));
    MinerActorState state{
        .precommitted_sectors = empty_hamt,
        .sectors = empty_amt,
        .fault_set = {},
        .proving_set = empty_amt,
        .info =
            {
                .owner = owner,
                .worker = worker,
                .pending_worker_key = boost::none,
                .peer_id = params2.peer_id,
                .sector_size = params2.sector_size,
            },
        .post_state =
            {
                .proving_period_start = kChainEpochUndefined,
                .num_consecutive_failures = 0,
            },
    };
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD(controlAddresses) {
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<MinerActorState>());
    OUTCOME_TRY(result,
                codec::cbor::encode(GetControlAddressesReturn{
                    state.info.owner, state.info.worker}));
    return InvocationOutput{Buffer{result}};
  }

  ACTOR_METHOD(changeWorkerAddress) {
    OUTCOME_TRY(params2, decodeActorParams<ChangeWorkerAddressParams>(params));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<MinerActorState>());
    if (runtime.getImmediateCaller() != state.info.owner) {
      return VMExitCode::MINER_ACTOR_WRONG_CALLER;
    }
    OUTCOME_TRY(worker, resolveWorkerAddress(runtime, params2.new_worker));
    auto effective_at = runtime.getCurrentEpoch() + kWorkerKeyChangeDelay;
    state.info.pending_worker_key = WorkerKeyChange{
        .new_worker = worker,
        .effective_at = effective_at,
    };
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(
        enrollCronEvent(runtime,
                        effective_at,
                        {
                            .event_type = CronEventType::WorkerKeyChange,
                            .sectors = boost::none,
                        }));
    return outcome::success();
  }

  ACTOR_METHOD(changePeerId) {
    OUTCOME_TRY(params2, decodeActorParams<ChangePeerIdParams>(params));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<MinerActorState>());
    state.info.peer_id = params2.new_id;
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD(submitWindowedPoSt) {
    OUTCOME_TRY(params2, decodeActorParams<SubmitWindowedPoStParams>(params));
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    if (runtime.getCurrentEpoch() > state.post_state.proving_period_start
                                        + kWindowedPostChallengeDuration) {
      return VMExitCode::MINER_ACTOR_POST_TOO_LATE;
    }
    if (runtime.getCurrentEpoch() <= state.post_state.proving_period_start) {
      return VMExitCode::MINER_ACTOR_POST_TOO_EARLY;
    }
    OUTCOME_TRY(verifyWindowedPoSt(runtime, state, params2));
    state.post_state.num_consecutive_failures = 0;
    state.post_state.proving_period_start += kProvingPeriod;
    state.proving_set = state.sectors;
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(runtime.send(
        kStoragePowerAddress, kOnMinerSurprisePoStSuccessMethodNumber, {}, 0));
    return outcome::success();
  }

  ACTOR_METHOD(onDeleteMiner) {
    if (runtime.getImmediateCaller() != kStoragePowerAddress) {
      return VMExitCode::MINER_ACTOR_WRONG_CALLER;
    }
    OUTCOME_TRY(runtime.deleteActor());
    return outcome::success();
  }

  ACTOR_METHOD(preCommitSector) {
    OUTCOME_TRY(params2, decodeActorParams<PreCommitSectorParams>(params));
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));

    OUTCOME_TRY(already_commited,
                Amt(runtime.getIpfsDatastore(), state.sectors)
                    .contains(params2.sector));
    if (already_commited) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }

    auto deposit = precommitDeposit(
        state.info.sector_size, params2.expiration - runtime.getCurrentEpoch());
    OUTCOME_TRY(confirmPaymentAndRefundChange(runtime, deposit));

    Hamt hamt_precommitted_sectors{runtime.getIpfsDatastore(),
                                   state.precommitted_sectors};
    OUTCOME_TRY(hamt_precommitted_sectors.setCbor(
        uvarintKey(params2.sector),
        SectorPreCommitOnChainInfo{
            .info = params2,
            .precommit_deposit = deposit,
            .precommit_epoch = runtime.getCurrentEpoch(),
        }));
    OUTCOME_TRY(hamt_precommitted_sectors.flush());
    state.precommitted_sectors = hamt_precommitted_sectors.cid();
    OUTCOME_TRY(runtime.commitState(state));

    if (params2.expiration <= runtime.getCurrentEpoch()) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }

    OUTCOME_TRY(duration, maxSealDuration(params2.registered_proof));
    OUTCOME_TRY(
        enrollCronEvent(runtime,
                        runtime.getCurrentEpoch() + duration + 1,
                        {
                            .event_type = CronEventType::PreCommitExpiry,
                            .sectors = RleBitset{params2.sector},
                            .registered_proof = params2.registered_proof,
                        }));

    return outcome::success();
  }

  ACTOR_METHOD(proveCommitSector) {
    OUTCOME_TRY(params2, decodeActorParams<ProveCommitSectorParams>(params));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<MinerActorState>());
    auto ipld = runtime.getIpfsDatastore();
    auto sector = params2.sector;

    OUTCOME_TRY(precommit,
                Hamt(ipld, state.precommitted_sectors)
                    .getCbor<SectorPreCommitOnChainInfo>(uvarintKey(sector)));

    OUTCOME_TRY(
        verifySeal(runtime,
                   state.info.sector_size,
                   {
                       .sealed_cid = precommit.info.sealed_cid,
                       .interactive_epoch =
                           precommit.precommit_epoch + kPreCommitChallengeDelay,
                       .registered_proof = precommit.info.registered_proof,
                       .proof = params2.proof,
                       .deals = precommit.info.deal_ids,
                       .sector = precommit.info.sector,
                       .seal_rand_epoch = precommit.info.seal_epoch,
                   }));

    OUTCOME_TRY(deal_weight,
                runtime.sendPR<DealWeight>(
                    kStorageMarketAddress,
                    kVerifyDealsOnSectorProveCommitMethodNumber,
                    VerifyDealsOnSectorProveCommitParams{
                        .deals = precommit.info.deal_ids,
                        .sector_expiry = precommit.info.expiration,
                    },
                    0));

    OUTCOME_TRY(pledge,
                runtime.sendPR<TokenAmount>(
                    kStoragePowerAddress,
                    kOnSectorProveCommitMethodNumber,
                    OnSectorProveCommitParameters{
                        .weight =
                            {
                                .sector_size = state.info.sector_size,
                                .duration = precommit.info.expiration
                                            - runtime.getCurrentEpoch(),
                                .deal_weight = deal_weight,
                            }},
                    0));

    Amt amt_sectors{ipld, state.sectors};
    OUTCOME_TRY(sectors, amt_sectors.count());
    OUTCOME_TRY(
        amt_sectors.setCbor(precommit.info.sector,
                            SectorOnChainInfo{
                                .info = precommit.info,
                                .activation_epoch = runtime.getCurrentEpoch(),
                                .deal_weight = deal_weight,
                                .pledge_requirement = pledge,
                                .declared_fault_epoch = {},
                                .declared_fault_duration = {},
                            }));
    OUTCOME_TRY(amt_sectors.flush());
    state.sectors = amt_sectors.cid();

    Hamt hamt_precommitted_sectors{ipld, state.precommitted_sectors};
    OUTCOME_TRY(hamt_precommitted_sectors.remove(uvarintKey(sector)));
    OUTCOME_TRY(hamt_precommitted_sectors.flush());
    state.precommitted_sectors = hamt_precommitted_sectors.cid();

    if (sectors == 1) {
      state.post_state.proving_period_start =
          runtime.getCurrentEpoch() + kProvingPeriod;
    }

    if (!inChallengeWindow(state, runtime)) {
      state.proving_set = state.sectors;
    }

    OUTCOME_TRY(runtime.commitState(state));

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

  ACTOR_METHOD(extendSectorExpiration) {
    OUTCOME_TRY(params2,
                decodeActorParams<ExtendSectorExpirationParams>(params));
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    Amt amt_sectors{runtime.getIpfsDatastore(), state.sectors};

    OUTCOME_TRY(sector, amt_sectors.getCbor<SectorOnChainInfo>(params2.sector));

    auto prev_weight{asStorageWeightDesc(state.info.sector_size, sector)};
    auto extension = params2.new_expiration - sector.info.expiration;
    if (extension < 0) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }
    auto new_weight{prev_weight};
    new_weight.duration = prev_weight.duration + extension;

    OUTCOME_TRY(new_pledge,
                runtime.sendPR<TokenAmount>(
                    kStoragePowerAddress,
                    kOnSectorModifyWeightDescMethodNumber,
                    OnSectorModifyWeightDescParams{
                        .prev_weight = prev_weight,
                        .prev_pledge = sector.pledge_requirement,
                        .new_weight = new_weight,
                    },
                    0));

    sector.info.expiration = params2.new_expiration;
    sector.pledge_requirement = new_pledge;
    OUTCOME_TRY(amt_sectors.setCbor(sector.info.sector, sector));
    OUTCOME_TRY(amt_sectors.flush());

    state.sectors = amt_sectors.cid();
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD(terminateSectors) {
    OUTCOME_TRY(params2, decodeActorParams<TerminateSectorsParams>(params));
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    if (!params2.sectors) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }
    OUTCOME_TRY(terminateSectorsInternal(
        runtime,
        state,
        *params2.sectors,
        SectorTerminationType::SECTOR_TERMINATION_MANUAL));
    return outcome::success();
  }

  ACTOR_METHOD(declareTemporaryFaults) {
    OUTCOME_TRY(params2,
                decodeActorParams<DeclareTemporaryFaultsParams>(params));
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    if (params2.duration <= 0) {
      return VMExitCode::MINER_ACTOR_ILLEGAL_ARGUMENT;
    }

    Amt amt_sectors{runtime.getIpfsDatastore(), state.sectors};
    auto effective_at =
        runtime.getCurrentEpoch() + kDeclaredFaultEffectiveDelay;
    std::vector<SectorStorageWeightDesc> weights;
    for (auto sector_num : params2.sectors) {
      if (state.fault_set.find(sector_num) != state.fault_set.end()) {
        continue;
      }
      OUTCOME_TRY(sector, amt_sectors.getCbor<SectorOnChainInfo>(sector_num));
      weights.push_back(asStorageWeightDesc(state.info.sector_size, sector));
      sector.declared_fault_epoch = effective_at;
      sector.declared_fault_duration = params2.duration;
      OUTCOME_TRY(amt_sectors.setCbor(sector_num, sector));
    }
    OUTCOME_TRY(amt_sectors.flush());
    state.sectors = amt_sectors.cid();
    OUTCOME_TRY(runtime.commitState(state));
    auto required_fee = temporaryFaultFee(weights, params2.duration);

    OUTCOME_TRY(confirmPaymentAndRefundChange(runtime, required_fee));
    OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, required_fee));

    CronEventPayload cron{
        .event_type = CronEventType::TempFault,
        .sectors = params2.sectors,
    };
    OUTCOME_TRY(enrollCronEvent(runtime, effective_at, cron));
    OUTCOME_TRY(
        enrollCronEvent(runtime, effective_at + params2.duration, cron));
    return outcome::success();
  }

  ACTOR_METHOD(onDeferredCronEvent) {
    if (runtime.getImmediateCaller() != kStoragePowerAddress) {
      return VMExitCode::MINER_ACTOR_WRONG_CALLER;
    }
    OUTCOME_TRY(params2, decodeActorParams<OnDeferredCronEventParams>(params));
    OUTCOME_TRY(
        payload,
        codec::cbor::decode<CronEventPayload>(params2.callback_payload));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<MinerActorState>());
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

  const ActorExports exports{
      {kConstructorMethodNumber, ActorMethod(constructor)},
      {kGetControlAddressesMethodNumber, ActorMethod(controlAddresses)},
      {kChangeWorkerAddressMethodNumber, ActorMethod(changeWorkerAddress)},
      {kChangePeerIdMethodNumber, ActorMethod(changePeerId)},
      {kSubmitWindowedPoStMethodNumber, ActorMethod(submitWindowedPoSt)},
      {kOnDeleteMinerMethodNumber, ActorMethod(onDeleteMiner)},
      {kPreCommitSectorMethodNumber, ActorMethod(preCommitSector)},
      {kProveCommitSectorMethodNumber, ActorMethod(proveCommitSector)},
      {kExtendSectorExpirationMethodNumber,
       ActorMethod(extendSectorExpiration)},
      {kTerminateSectorsMethodNumber, ActorMethod(terminateSectors)},
      {kDeclareTemporaryFaultsMethodNumber, declareTemporaryFaults},
      {kOnDeferredCronEventMethodNumber, onDeferredCronEvent},
  };
}  // namespace fc::vm::actor::builtin::miner
