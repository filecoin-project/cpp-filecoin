/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/miner/miner_actor.hpp"

#include <boost/endian/buffers.hpp>

#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/actor/builtin/market/actor.hpp"
#include "vm/actor/builtin/miner/policy.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::miner {
  using primitives::address::Protocol;
  using primitives::sector::OnChainSealVerifyInfo;
  using primitives::sector::SectorInfo;
  using runtime::DomainSeparationTag;
  using storage_power::SectorTerminationType;

  outcome::result<void> burnFunds(Runtime &runtime, const TokenAmount &amount) {
    if (amount > 0) {
      OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, amount));
    }
    return outcome::success();
  }

  outcome::result<void> notifyPledgeChanged(Runtime &runtime,
                                            const TokenAmount &pledge) {
    if (pledge != 0) {
      OUTCOME_TRY(runtime.sendM<storage_power::UpdatePledgeTotal>(
          kStoragePowerAddress, pledge, 0));
    }
    return outcome::success();
  }

  outcome::result<void> burnFundsAndNotifyPledgeChange(
      Runtime &runtime, const TokenAmount &amount) {
    OUTCOME_TRY(burnFunds(runtime, amount));
    OUTCOME_TRY(notifyPledgeChanged(runtime, -amount));
    return outcome::success();
  }

  ChainEpoch assignProvingPeriodOffset(const Address &miner, ChainEpoch now) {
    OUTCOME_EXCEPT(seed, codec::cbor::encode(miner));
    seed.putUint64(static_cast<uint64_t>(now));
    auto hash = Runtime::hashBlake2b(seed);
    boost::endian::big_uint64_buf_t offset;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::copy_n(
        hash.begin(), sizeof(offset), reinterpret_cast<uint8_t *>(&offset));
    return offset.value() % kWPoStProvingPeriod;
  }

  ChainEpoch nextProvingPeriodStart(ChainEpoch now, ChainEpoch offset) {
    auto progress{now % kWPoStProvingPeriod - offset};
    if (progress < 0) {
      progress += kWPoStProvingPeriod;
    }
    auto period_start{now - progress + kWPoStProvingPeriod};
    assert(period_start > now);
    return period_start;
  }

  outcome::result<ChainEpoch> sealChallengeEarliest(ChainEpoch now,
                                                    RegisteredProof proof) {
    OUTCOME_TRY(max, maxSealDuration(proof));
    return now - kChainFinalityish - max;
  }

  outcome::result<void> addPreCommitDeposit(State &state,
                                            const TokenAmount &amount) {
    state.precommit_deposit += amount;
    VM_ASSERT(state.precommit_deposit >= 0);
    return outcome::success();
  }

  ChainEpoch quantizeUp(ChainEpoch epoch, ChainEpoch unit) {
    auto rem{epoch % unit};
    return rem == 0 ? epoch : epoch - rem + unit;
  }

  outcome::result<void> addLockedFunds(State &state,
                                       ChainEpoch now,
                                       const TokenAmount &vesting_sum) {
    constexpr EpochDuration kInitialDelay{7 * kEpochsInDay};
    constexpr EpochDuration kVestPeriod{7 * kEpochsInDay};
    constexpr EpochDuration kStepDuration{kEpochsInDay};
    constexpr EpochDuration kQuantization{12 * kEpochsInHour};

    VM_ASSERT(vesting_sum >= 0);
    auto vest_begin{now + kInitialDelay};
    TokenAmount vested{0};
    for (auto epoch{vest_begin + kStepDuration}; vested < vesting_sum;
         epoch += kStepDuration) {
      auto vest_epoch{quantizeUp(epoch, kQuantization)};
      auto elapsed{vest_epoch - vest_begin};
      auto target{elapsed < kVestPeriod
                      ? bigdiv(vesting_sum * elapsed, kVestPeriod)
                      : vesting_sum};
      TokenAmount vest_this_time{target - vested};
      vested = target;
      OUTCOME_TRY(entry, state.vesting_funds.tryGet(vest_epoch));
      if (!entry) {
        entry = 0;
      }
      OUTCOME_TRY(state.vesting_funds.set(vest_epoch, *entry + vest_this_time));
    }
    state.locked_funds += vesting_sum;
    return outcome::success();
  }

  outcome::result<TokenAmount> unlockVestedFunds(State &state, ChainEpoch now) {
    TokenAmount unlocked{0};
    std::vector<ChainEpoch> deleted;
    OUTCOME_TRY(state.vesting_funds.visit([&](auto epoch, auto &locked) {
      // TODO: lotus stops iterations
      if (static_cast<ChainEpoch>(epoch) < now) {
        unlocked += locked;
        deleted.push_back(epoch);
      }
      return outcome::success();
    }));
    for (auto epoch : deleted) {
      OUTCOME_TRY(state.vesting_funds.remove(epoch));
    }
    state.locked_funds -= unlocked;
    VM_ASSERT(state.locked_funds >= 0);
    return std::move(unlocked);
  }

  TokenAmount getAvailableBalance(const State &state,
                                  const TokenAmount &actor) {
    TokenAmount available{actor - state.locked_funds - state.precommit_deposit};
    assert(available >= 0);
    return available;
  }

  outcome::result<void> assertBalanceInvariants(const State &state,
                                                const TokenAmount &actor) {
    VM_ASSERT(state.precommit_deposit >= 0);
    VM_ASSERT(state.locked_funds >= 0);
    VM_ASSERT(actor >= state.precommit_deposit + state.locked_funds);
    return outcome::success();
  }

  outcome::result<void> addNewSectors(State &state, SectorNumber sector) {
    state.new_sectors.insert(sector);
    VM_ASSERT(state.new_sectors.size() <= kNewSectorsPerPeriodMax);
    return outcome::success();
  }

  outcome::result<void> addSectorExpirations(State &state,
                                             ChainEpoch expiration,
                                             SectorNumber sector) {
    OUTCOME_TRY(sectors, state.sector_expirations.tryGet(expiration));
    if (!sectors) {
      sectors = RleBitset{};
    }
    sectors->insert(sector);
    VM_ASSERT(sectors->size() <= kSectorsMax);
    OUTCOME_TRY(state.sector_expirations.set(expiration, *sectors));
    return outcome::success();
  }

  outcome::result<void> removeFaults(State &state, const RleBitset &sectors) {
    for (auto sector : sectors) {
      state.fault_set.erase(sector);
    }
    OUTCOME_TRY(state.fault_epochs.visit(
        [&](auto epoch, auto faults) -> outcome::result<void> {
          auto change{false};
          for (auto sector : sectors) {
            if (faults.erase(sector) != 0) {
              change = true;
            }
          }
          if (change) {
            OUTCOME_TRY(state.fault_epochs.set(epoch, faults));
          }
          return outcome::success();
        }));
    return outcome::success();
  }

  outcome::result<void> removeTerminatedSectors(State &state,
                                                Deadlines &deadlines,
                                                const RleBitset &sectors) {
    for (auto sector : sectors) {
      OUTCOME_TRY(state.sectors.remove(sector));
      state.new_sectors.erase(sector);
      for (auto &deadline : deadlines.due) {
        deadline.erase(sector);
      }
      state.recoveries.erase(sector);
    }
    OUTCOME_TRY(removeFaults(state, sectors));
    return outcome::success();
  }

  DeadlineInfo DeadlineInfo::make(ChainEpoch start,
                                  size_t index,
                                  ChainEpoch now) {
    if (index < kWPoStPeriodDeadlines) {
      auto open{start + static_cast<ChainEpoch>(index) * kWPoStChallengeWindow};
      return {now,
              start,
              index,
              open,
              open + kWPoStChallengeWindow,
              open - kWPoStChallengeLookback,
              open - kFaultDeclarationCutoff};
    }
    auto after{start + kWPoStProvingPeriod};
    return {now, start, index, after, after, after, 0};
  }

  DeadlineInfo State::deadlineInfo(ChainEpoch now) const {
    auto progress{now - proving_period_start};
    size_t deadline{kWPoStPeriodDeadlines};
    if (progress < kWPoStProvingPeriod) {
      deadline = std::max(ChainEpoch{0}, progress / kWPoStChallengeWindow);
    }
    return DeadlineInfo::make(proving_period_start, deadline, now);
  }

  outcome::result<void> State::addFaults(const RleBitset &sectors,
                                         ChainEpoch epoch) {
    if (sectors.empty()) {
      return outcome::success();
    }
    fault_set.insert(sectors.begin(), sectors.end());
    VM_ASSERT(fault_set.size() <= kSectorsMax);
    OUTCOME_TRY(faults, fault_epochs.tryGet(epoch));
    if (!faults) {
      faults = RleBitset{};
    }
    faults->insert(sectors.begin(), sectors.end());
    OUTCOME_TRY(fault_epochs.set(epoch, *faults));
    return outcome::success();
  }

  auto computeFaultsFromMissingPoSts(const State &state,
                                     const Deadlines &deadlines,
                                     size_t before_deadline) {
    std::pair<RleBitset, RleBitset> faults;
    auto part_size{state.info.window_post_partition_sectors};
    size_t first_part{0};
    for (size_t deadline{0}; deadline < before_deadline; ++deadline) {
      auto [parts, sectors]{deadlines.count(part_size, deadline)};
      auto sector{deadlines.due[deadline].begin()};
      for (size_t part{0}; part < parts; ++part) {
        if (state.post_submissions.find(first_part + part)
            == state.post_submissions.end()) {
          for (auto i{std::min(part_size, sectors - part * part_size)}; i != 0;
               --i) {
            if (state.fault_set.find(*sector) == state.fault_set.end()) {
              faults.first.insert(*sector);
            }
            if (state.recoveries.find(*sector) != state.recoveries.end()) {
              faults.second.insert(*sector);
            }
            ++sector;
          }
        }
      }
      first_part += parts;
    }
    return faults;
  }

  outcome::result<RleBitset> computePartitionsSectors(
      const Deadlines &deadlines,
      size_t part_size,
      size_t index,
      const std::vector<uint64_t> parts) {
    RleBitset result;
    auto [first_part, sectors]{deadlines.partitions(part_size, index)};
    auto max_part{first_part + (sectors + part_size - 1) / part_size};
    for (auto part : parts) {
      VM_ASSERT(part >= first_part && part < max_part);
      size_t offset{(part - first_part) * part_size};
      auto begin{deadlines.due[index].begin()};
      std::advance(begin, offset);
      auto end{begin};
      std::advance(end, std::min(part_size, sectors - offset));
      result.insert(begin, end);
    }
    return std::move(result);
  }

  outcome::result<std::pair<std::vector<SectorOnChainInfo>, TokenAmount>>
  checkMissingPoStFaults(State &state,
                         const Deadlines &deadlines,
                         size_t before_deadline,
                         ChainEpoch period_start) {
    auto [detected, recoveries] =
        computeFaultsFromMissingPoSts(state, deadlines, before_deadline);
    OUTCOME_TRY(state.addFaults(detected, period_start));
    for (auto sector : recoveries) {
      state.recoveries.erase(sector);
    }
    std::vector<SectorOnChainInfo> detected_sectors;
    for (auto sector_num : detected) {
      OUTCOME_TRY(sector, state.sectors.get(sector_num));
      detected_sectors.push_back(std::move(sector));
    }
    // TODO(turuslan): zero penalty now, FIL-233
    TokenAmount penalty{0};
    return std::make_pair(std::move(detected_sectors), std::move(penalty));
  }

  DeadlineInfo declarationDeadlineInfo(ChainEpoch period_start,
                                       size_t index,
                                       ChainEpoch now) {
    DeadlineInfo info{DeadlineInfo::make(period_start, index, now)};
    if (info.elapsed()) {
      return DeadlineInfo::make(info.nextPeriodStart(), index, now);
    }
    return info;
  }

  outcome::result<void> validateFRDeclaration(const Deadlines &deadlines,
                                              const DeadlineInfo &info,
                                              const RleBitset &declared) {
    VM_ASSERT(!info.faultCutoffPassed());
    auto &sectors{deadlines.due[info.index]};
    for (auto sector : declared) {
      VM_ASSERT(sectors.count(sector) != 0);
    }
    return outcome::success();
  }

  outcome::result<std::pair<std::vector<SectorOnChainInfo>, RleBitset>>
  loadSectorInfosForProof(State &state, const RleBitset &proven) {
    std::pair<std::vector<SectorOnChainInfo>, RleBitset> result;
    result.first.resize(proven.size());
    std::vector<size_t> faults;
    size_t i{0};
    boost::optional<size_t> good;
    for (auto &s : proven) {
      auto fault{state.fault_set.count(s) != 0};
      auto recovery{fault && state.recoveries.count(s) != 0};
      if (recovery) {
        result.second.insert(s);
      }
      if (!fault || recovery) {
        if (!good) {
          good = i;
        }
        OUTCOME_TRYA(result.first[i], state.sectors.get(s));
      } else {
        faults.push_back(i);
      }
    }
    VM_ASSERT(good);
    for (auto i : faults) {
      result.first[i] = result.first[*good];
    }
    return std::move(result);
  }

  auto loadState(Runtime &runtime) {
    return runtime.getCurrentActorStateCbor<State>();
  }

  outcome::result<MinerActorState> assertCallerIsWorker(Runtime &runtime) {
    OUTCOME_TRY(state, loadState(runtime));
    OUTCOME_TRY(runtime.validateImmediateCallerIs(state.info.worker));
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
      return VMExitCode::kMinerActorOwnerNotSignable;
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
      return VMExitCode::kMinerActorNotAccount;
    }
    if (address.getProtocol() != Protocol::BLS) {
      OUTCOME_TRY(key, runtime.sendM<account::PubkeyAddress>(id, {}, 0));
      if (key.getProtocol() != Protocol::BLS) {
        return VMExitCode::kMinerActorMinerNotBls;
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
            .payload = payload2,
        },
        0));
    return outcome::success();
  }

  outcome::result<void> verifyWindowedPoSt(
      Runtime &runtime,
      ChainEpoch challenge,
      const std::vector<SectorOnChainInfo> &sectors,
      const std::vector<PoStProof> &proofs) {
    OUTCOME_TRY(miner, runtime.resolveAddress(runtime.getCurrentReceiver()));
    OUTCOME_TRY(seed, codec::cbor::encode(miner));
    OUTCOME_TRY(
        randomness,
        runtime.getRandomness(
            DomainSeparationTag::WindowedPoStChallengeSeed, challenge, seed));
    primitives::sector::WindowPoStVerifyInfo params{
        .randomness = randomness,
        .proofs = proofs,
        .challenged_sectors = {},
        .prover = miner.getId(),
    };
    for (auto &sector : sectors) {
      params.challenged_sectors.push_back({
          sector.info.registered_proof,
          sector.info.sector,
          sector.info.sealed_cid,
      });
    }
    OUTCOME_TRY(verified, runtime.verifyPoSt(params));
    if (!verified) {
      return VMExitCode::kMinerActorIllegalArgument;
    }
    return outcome::success();
  }

  outcome::result<void> verifySeal(Runtime &runtime,
                                   const OnChainSealVerifyInfo &info) {
    ChainEpoch current_epoch = runtime.getCurrentEpoch();
    if (current_epoch <= info.interactive_epoch) {
      return VMExitCode::kMinerActorWrongEpoch;
    }

    OUTCOME_TRY(duration, maxSealDuration(info.registered_proof));
    if (info.seal_rand_epoch < current_epoch - kChainFinalityish - duration) {
      return VMExitCode::kMinerActorIllegalArgument;
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
    OUTCOME_TRY(seed, codec::cbor::encode(miner));
    OUTCOME_TRY(
        randomness,
        runtime.getRandomness(
            DomainSeparationTag::SealRandomness, info.seal_rand_epoch, seed));
    OUTCOME_TRY(
        interactive_randomness,
        runtime.getRandomness(DomainSeparationTag::InteractiveSealChallengeSeed,
                              info.interactive_epoch,
                              seed));
    OUTCOME_TRY(runtime.verifySeal({
        .sector =
            {
                .miner = miner.getId(),
                .sector = info.sector,
            },
        .info = info,
        .randomness = randomness,
        .interactive_randomness = interactive_randomness,
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

  template <typename M>
  outcome::result<void> requestWithWeights(
      Runtime &runtime,
      typename M::Params params,
      SectorSize sector_size,
      const std::vector<SectorOnChainInfo> &sectors) {
    if (!sectors.empty()) {
      for (auto &sector : sectors) {
        params.weights.push_back(asStorageWeightDesc(sector_size, sector));
      }
      OUTCOME_TRY(runtime.sendM<M>(kStoragePowerAddress, params, 0));
    }
    return outcome::success();
  }

  outcome::result<void> requestTerminatePower(
      Runtime &runtime,
      SectorTerminationType type,
      SectorSize sector_size,
      const std::vector<SectorOnChainInfo> &sectors) {
    return requestWithWeights<storage_power::OnSectorTerminate>(
        runtime, {type, {}}, sector_size, sectors);
  }

  outcome::result<void> requestTerminateDeals(
      Runtime &runtime, const std::vector<DealId> &deals) {
    if (deals.empty()) {
      return outcome::success();
    }
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
      SectorSize sector_size,
      const std::vector<SectorOnChainInfo> &sectors) {
    return requestWithWeights<storage_power::OnFaultBegin>(
        runtime, {}, sector_size, sectors);
  }

  outcome::result<void> requestEndFaults(
      Runtime &runtime,
      SectorSize sector_size,
      const std::vector<SectorOnChainInfo> &sectors) {
    return requestWithWeights<storage_power::OnFaultEnd>(
        runtime, {}, sector_size, sectors);
  }

  outcome::result<RleBitset> popSectorExpirations(State &state,
                                                  ChainEpoch epoch) {
    RleBitset result;
    std::vector<ChainEpoch> expired;
    OUTCOME_TRY(state.sector_expirations.visit([&](auto expiry, auto &sectors) {
      // TODO: lotus stops iterations
      if (static_cast<ChainEpoch>(expiry) <= epoch) {
        expired.push_back(expiry);
        result.insert(sectors.begin(), sectors.end());
      }
      return outcome::success();
    }));
    for (auto expiry : expired) {
      OUTCOME_TRY(state.sector_expirations.remove(expiry));
    }
    return std::move(result);
  }

  outcome::result<RleBitset> popExpiredFaults(State &state, ChainEpoch latest) {
    RleBitset expired_sectors;
    std::vector<ChainEpoch> expired_epochs;
    OUTCOME_TRY(state.fault_epochs.visit([&](auto start, auto &sectors) {
      if (static_cast<ChainEpoch>(start) <= latest) {
        expired_epochs.push_back(start);
        expired_sectors.insert(sectors.begin(), sectors.end());
      }
      return outcome::success();
    }));
    for (auto epoch : expired_epochs) {
      OUTCOME_TRY(state.fault_epochs.remove(epoch));
    }
    return std::move(expired_sectors);
  }

  void assignNewSectors(Deadlines &deadlines,
                        size_t part_size,
                        const RleBitset &available) {
    auto begin{available.begin()};
    auto add = [&](size_t i, size_t n) {
      auto end{begin};
      while (n != 0 && begin != available.end()) {
        --n;
        ++end;
      }
      deadlines.due[i].insert(begin, end);
      begin = end;
    };
    for (size_t i{0}; i < deadlines.due.size() && begin != available.end();
         ++i) {
      auto [parts, sectors]{deadlines.count(part_size, i)};
      auto mod{sectors % part_size};
      if (mod != 0) {
        add(i, part_size - mod);
      }
    }
    for (size_t i{0}; begin != available.end();
         i = (i + 1) % deadlines.due.size()) {
      add(i, part_size);
    }
  }

  outcome::result<void> terminateSectorsInternal(Runtime &runtime,
                                                 MinerActorState &state,
                                                 const RleBitset &sectors,
                                                 SectorTerminationType type) {
    if (sectors.empty()) {
      return outcome::success();
    }
    std::vector<DealId> deals;
    std::vector<SectorOnChainInfo> all_sectors, faults;
    for (auto sector_num : sectors) {
      OUTCOME_TRY(sector, state.sectors.get(sector_num));
      deals.insert(deals.end(),
                   sector.info.deal_ids.begin(),
                   sector.info.deal_ids.end());
      all_sectors.push_back(sector);
      if (state.fault_set.find(sector_num) != state.fault_set.end()) {
        faults.push_back(sector);
      }
    }
    OUTCOME_TRY(deadlines, state.getDeadlines(runtime));
    OUTCOME_TRY(removeTerminatedSectors(state, deadlines, sectors));
    OUTCOME_TRYA(state.deadlines,
                 runtime.getIpfsDatastore()->setCbor(deadlines));
    TokenAmount penalty{0};
    if (type != SectorTerminationType::EXPIRED) {
      // TODO(turuslan): zero penalty now, FIL-233
    }
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(requestEndFaults(runtime, state.info.sector_size, faults));
    OUTCOME_TRY(requestTerminateDeals(runtime, deals));
    OUTCOME_TRY(requestTerminatePower(
        runtime, type, state.info.sector_size, all_sectors));
    OUTCOME_TRY(burnFundsAndNotifyPledgeChange(runtime, penalty));
    return outcome::success();
  }

  outcome::result<void> checkPrecommitExpiry(Runtime &runtime,
                                             MinerActorState &state,
                                             const RleBitset &sectors) {
    TokenAmount to_burn;
    for (auto sector_num : sectors) {
      OUTCOME_TRY(precommit, state.precommitted_sectors.tryGet(sector_num));
      if (!precommit) {
        continue;
      }
      OUTCOME_TRY(state.precommitted_sectors.remove(sector_num));
      to_burn += precommit->precommit_deposit;
    }
    state.precommit_deposit -= to_burn;
    VM_ASSERT(state.precommit_deposit >= 0);
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(burnFunds(runtime, to_burn));
    return outcome::success();
  }

  outcome::result<void> commitWorkerKeyChange(Runtime &runtime,
                                              MinerActorState &state) {
    if (!state.info.pending_worker_key) {
      return VMExitCode::kMinerActorIllegalState;
    }
    if (state.info.pending_worker_key->effective_at
        > runtime.getCurrentEpoch()) {
      return VMExitCode::kMinerActorIllegalState;
    }
    state.info.worker = state.info.pending_worker_key->new_worker;
    state.info.pending_worker_key = boost::none;
    return runtime.commitState(state);
  }

  outcome::result<void> handleProvingPeriod(Runtime &runtime, State &state) {
    auto now{runtime.getCurrentEpoch()};
    auto deadline{state.deadlineInfo(now)};
    OUTCOME_TRY(deadlines, state.getDeadlines(runtime));

    OUTCOME_TRY(new_vest, unlockVestedFunds(state, now));
    OUTCOME_TRY(notifyPledgeChanged(runtime, -new_vest));

    if (deadline.periodStarted()) {
      OUTCOME_TRY(
          detected_penalty,
          checkMissingPoStFaults(
              state, deadlines, deadline.period_start, kWPoStPeriodDeadlines));
      OUTCOME_TRY(requestBeginFaults(
          runtime, state.info.sector_size, detected_penalty.first));
      OUTCOME_TRY(
          burnFundsAndNotifyPledgeChange(runtime, detected_penalty.second));
    }

    OUTCOME_TRY(expired_sectors,
                popSectorExpirations(state, deadline.periodEnd()));
    OUTCOME_TRY(terminateSectorsInternal(
        runtime, state, expired_sectors, SectorTerminationType::EXPIRED));

    OUTCOME_TRY(expired_faults,
                popExpiredFaults(state, deadline.periodEnd() - kFaultMaxAge));
    // TODO(turuslan): zero penalty now, FIL-233
    TokenAmount ongoing_penalty{0};
    OUTCOME_TRY(terminateSectorsInternal(
        runtime, state, expired_faults, SectorTerminationType::FAULTY));
    OUTCOME_TRY(burnFundsAndNotifyPledgeChange(runtime, ongoing_penalty));

    if (!state.new_sectors.empty()) {
      assignNewSectors(deadlines,
                       state.info.window_post_partition_sectors,
                       state.new_sectors);
      OUTCOME_TRYA(state.deadlines,
                   runtime.getIpfsDatastore()->setCbor(deadlines));
      state.new_sectors.clear();
    }
    state.post_submissions.clear();
    if (deadline.periodStarted()) {
      state.proving_period_start += kWPoStProvingPeriod;
    }

    OUTCOME_TRY(runtime.commitState(state));

    OUTCOME_TRY(
        enrollCronEvent(runtime,
                        state.proving_period_start + kWPoStProvingPeriod - 1,
                        {CronEventType::ProvingPeriod, boost::none}));

    return outcome::success();
  }

  ACTOR_METHOD_IMPL(Construct) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kInitAddress));
    // TODO: seal supported
    OUTCOME_TRY(owner, resolveOwnerAddress(runtime, params.owner));
    OUTCOME_TRY(worker, resolveWorkerAddress(runtime, params.worker));
    Deadlines deadlines;
    deadlines.due.resize(kWPoStPeriodDeadlines);
    auto ipld{runtime.getIpfsDatastore()};
    OUTCOME_TRY(deadlines_cid, ipld->setCbor(deadlines));

    auto now{runtime.getCurrentEpoch()};
    auto period_start{nextProvingPeriodStart(
        now, assignProvingPeriodOffset(runtime.getCurrentReceiver(), now))};
    VM_ASSERT(period_start > now);

    OUTCOME_TRY(
        seal_proof_type,
        primitives::sector::getRegisteredSealProof(params.seal_proof_type));
    OUTCOME_TRY(sector_size,
                primitives::sector::getSectorSize(seal_proof_type));
    OUTCOME_TRY(
        partition_sectors,
        primitives::sector::getWindowPoStPartitionSectors(seal_proof_type));
    MinerActorState state{
        .info =
            {
                .owner = owner,
                .worker = worker,
                .pending_worker_key = boost::none,
                .peer_id = params.peer_id,
                .seal_proof_type = seal_proof_type,
                .sector_size = sector_size,
                .window_post_partition_sectors = partition_sectors,
            },
        .precommit_deposit = 0,
        .locked_funds = 0,
        .vesting_funds = {},
        .precommitted_sectors = {},
        .sectors = {},
        .proving_period_start = period_start,
        .new_sectors = {},
        .sector_expirations = {},
        .deadlines = deadlines_cid,
        .fault_set = {},
        .fault_epochs = {},
        .recoveries = {},
        .post_submissions = {},
    };
    ipld->load(state);
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(enrollCronEvent(runtime,
                                period_start - 1,
                                {CronEventType::ProvingPeriod, boost::none}));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ControlAddresses) {
    OUTCOME_TRY(state, loadState(runtime));
    return Result{state.info.owner, state.info.worker};
  }

  ACTOR_METHOD_IMPL(ChangeWorkerAddress) {
    OUTCOME_TRY(state, loadState(runtime));
    OUTCOME_TRY(runtime.validateImmediateCallerIs(state.info.owner));
    OUTCOME_TRY(worker, resolveWorkerAddress(runtime, params.new_worker));
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

  ACTOR_METHOD_IMPL(ChangePeerId) {
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    state.info.peer_id = params.new_id;
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(SubmitWindowedPoSt) {
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    auto now{runtime.getCurrentEpoch()};
    VM_ASSERT(params.partitions.size() <= windowPoStMessagePartitionsMax(
                  state.info.window_post_partition_sectors));
    auto deadline = state.deadlineInfo(now);
    VM_ASSERT(deadline.periodStarted());
    VM_ASSERT(!deadline.elapsed());
    VM_ASSERT(params.deadline == deadline.index);
    OUTCOME_TRY(deadlines, state.getDeadlines(runtime));
    OUTCOME_TRY(detected_penalty,
                checkMissingPoStFaults(
                    state, deadlines, deadline.index, deadline.period_start));
    OUTCOME_TRY(
        part_sectors,
        computePartitionsSectors(deadlines,
                                 state.info.window_post_partition_sectors,
                                 deadline.index,
                                 params.partitions));
    OUTCOME_TRY(info, loadSectorInfosForProof(state, part_sectors));
    OUTCOME_TRY(verifyWindowedPoSt(
        runtime, deadline.challenge, info.first, params.proofs));
    for (auto part : params.partitions) {
      VM_ASSERT(state.post_submissions.insert(part).second);
    }
    OUTCOME_TRY(removeFaults(state, info.second));
    for (auto sector : info.second) {
      state.recoveries.erase(sector);
    }
    OUTCOME_TRY(recovered_sectors, state.getSectors(info.second));
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(requestBeginFaults(
        runtime, state.info.sector_size, detected_penalty.first));
    OUTCOME_TRY(
        burnFundsAndNotifyPledgeChange(runtime, detected_penalty.second));
    OUTCOME_TRY(
        requestEndFaults(runtime, state.info.sector_size, recovered_sectors));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(PreCommitSector) {
    auto now{runtime.getCurrentEpoch()};
    VM_ASSERT(params.expiration > now);
    VM_ASSERT(params.seal_epoch < now);
    OUTCOME_TRY(earliest, sealChallengeEarliest(now, params.registered_proof));
    VM_ASSERT(params.seal_epoch >= earliest);
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    VM_ASSERT(params.registered_proof == state.info.seal_proof_type);
    OUTCOME_TRY(already_precommited,
                state.precommitted_sectors.has(params.sector));
    VM_ASSERT(!already_precommited);
    OUTCOME_TRY(already_commited, state.sectors.has(params.sector));
    VM_ASSERT(!already_commited);
    VM_ASSERT((params.expiration + 1) % kWPoStProvingPeriod
              == state.proving_period_start % kWPoStProvingPeriod);
    OUTCOME_TRY(new_vest, unlockVestedFunds(state, now));
    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    auto deposit{
        precommitDeposit(state.info.sector_size, params.expiration - now)};
    VM_ASSERT(getAvailableBalance(state, balance) >= deposit);
    OUTCOME_TRY(addPreCommitDeposit(state, deposit));
    OUTCOME_TRY(
        state.precommitted_sectors.set(params.sector, {params, deposit, now}));
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(notifyPledgeChanged(runtime, -new_vest));
    OUTCOME_TRY(duration, maxSealDuration(params.registered_proof));
    OUTCOME_TRY(enrollCronEvent(
        runtime,
        now + duration + 1,
        {CronEventType::PreCommitExpiry, RleBitset{params.sector}}));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ProveCommitSector) {
    auto now{runtime.getCurrentEpoch()};
    OUTCOME_TRY(state, loadState(runtime));
    auto sector = params.sector;

    OUTCOME_TRY(precommit, state.precommitted_sectors.get(sector));
    OUTCOME_TRY(duration, maxSealDuration(precommit.info.registered_proof));
    VM_ASSERT(precommit.precommit_epoch + duration >= now);

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
                                .duration = precommit.info.expiration - now,
                                .deal_weight = deal_weight.deal_weight,
                                .verified_deal_weight =
                                    deal_weight.verified_deal_weight,
                            },
                    },
                    0));

    OUTCOME_TRY(new_vest, unlockVestedFunds(state, now));
    OUTCOME_TRY(addPreCommitDeposit(state, -precommit.precommit_deposit));
    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    VM_ASSERT(getAvailableBalance(state, balance) >= pledge);
    OUTCOME_TRY(addLockedFunds(state, now, pledge));
    OUTCOME_TRY(assertBalanceInvariants(state, balance));

    OUTCOME_TRY(state.sectors.set(
        precommit.info.sector,
        SectorOnChainInfo{
            .info = precommit.info,
            .activation_epoch = now,
            .deal_weight = deal_weight.deal_weight,
            .verified_deal_weight = deal_weight.verified_deal_weight,
        }));

    OUTCOME_TRY(state.precommitted_sectors.remove(sector));
    OUTCOME_TRY(addSectorExpirations(state, precommit.info.expiration, sector));
    OUTCOME_TRY(addNewSectors(state, sector));
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(notifyPledgeChanged(runtime, pledge - new_vest));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ExtendSectorExpiration) {
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));

    OUTCOME_TRY(sector, state.sectors.get(params.sector));

    auto prev_weight{asStorageWeightDesc(state.info.sector_size, sector)};
    auto extension = params.new_expiration - sector.info.expiration;
    VM_ASSERT(extension >= 0);
    auto new_weight{prev_weight};
    new_weight.duration = prev_weight.duration + extension;

    OUTCOME_TRY(runtime.sendM<storage_power::OnSectorModifyWeightDesc>(
        kStoragePowerAddress,
        {
            .prev_weight = prev_weight,
            .new_weight = new_weight,
        },
        0));

    sector.info.expiration = params.new_expiration;
    OUTCOME_TRY(state.sectors.set(sector.info.sector, sector));

    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(TerminateSectors) {
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    OUTCOME_TRY(terminateSectorsInternal(
        runtime, state, params.sectors, SectorTerminationType::MANUAL));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(DeclareFaults) {
    VM_ASSERT(params.faults.size() <= kWPoStPeriodDeadlines);
    auto now{runtime.getCurrentEpoch()};
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    auto deadline{state.deadlineInfo(now)};
    OUTCOME_TRY(deadlines, state.getDeadlines(runtime));
    OUTCOME_TRY(faults_penalty,
                checkMissingPoStFaults(
                    state, deadlines, deadline.index, deadline.period_start));
    RleBitset new_faults;
    for (auto &fault : params.faults) {
      OUTCOME_TRY(validateFRDeclaration(
          deadlines,
          declarationDeadlineInfo(
              state.proving_period_start, fault.deadline, now),
          fault.sectors));
      for (auto sector : fault.sectors) {
        if (state.recoveries.erase(sector) == 0) {
          VM_ASSERT(state.fault_set.find(sector) == state.fault_set.end());
          new_faults.insert(sector);
        }
      }
    }
    OUTCOME_TRY(state.addFaults(new_faults, state.proving_period_start));
    for (auto sector_num : new_faults) {
      OUTCOME_TRY(sector, state.sectors.get(sector_num));
      faults_penalty.first.push_back(std::move(sector));
    }
    // TODO(turuslan): zero penalty now, FIL-233
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(requestBeginFaults(
        runtime, state.info.sector_size, faults_penalty.first));
    OUTCOME_TRY(burnFundsAndNotifyPledgeChange(runtime, faults_penalty.second));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(DeclareFaultsRecovered) {
    VM_ASSERT(params.recoveries.size() <= kWPoStPeriodDeadlines);
    OUTCOME_TRY(state, assertCallerIsWorker(runtime));
    OUTCOME_TRY(deadlines, state.getDeadlines(runtime));
    for (auto &recovery : params.recoveries) {
      OUTCOME_TRY(validateFRDeclaration(
          deadlines,
          declarationDeadlineInfo(state.proving_period_start,
                                  recovery.deadline,
                                  runtime.getCurrentEpoch()),
          recovery.sectors));
      for (auto sector : recovery.sectors) {
        VM_ASSERT(state.fault_set.count(sector) != 0);
        VM_ASSERT(state.recoveries.insert(sector).second);
      }
    }
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(OnDeferredCronEvent) {
    OUTCOME_TRY(runtime.validateImmediateCallerIs(kStoragePowerAddress));
    OUTCOME_TRY(state, loadState(runtime));
    switch (params.event_type) {
      case CronEventType::ProvingPeriod: {
        OUTCOME_TRY(handleProvingPeriod(runtime, state));
        break;
      }
      case CronEventType::PreCommitExpiry: {
        VM_ASSERT(params.sectors);
        OUTCOME_TRY(checkPrecommitExpiry(runtime, state, *params.sectors));
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
      return VMExitCode::kMinerActorNotFound;
    }
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AddLockedFund) {
    OUTCOME_TRY(state, loadState(runtime));
    auto caller{runtime.getImmediateCaller()};
    if (caller != kRewardAddress && caller != state.info.owner
        && caller != state.info.worker) {
      return VMExitCode::kSysErrForbidden;
    }
    auto now{runtime.getCurrentEpoch()};
    OUTCOME_TRY(new_vest, unlockVestedFunds(state, now));
    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    VM_ASSERT(params <= getAvailableBalance(state, balance));
    OUTCOME_TRY(addLockedFunds(state, now, params));
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(notifyPledgeChanged(runtime, params - new_vest));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(ReportConsensusFault) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsSignable());
    OUTCOME_TRY(fault,
                runtime.verifyConsensusFault(
                    params.block1, params.block2, params.extra));
    auto age{runtime.getCurrentEpoch() - fault.epoch};
    VM_ASSERT(age > 0);
    OUTCOME_TRY(state, loadState(runtime));
    OUTCOME_TRY(runtime.sendM<storage_power::OnConsensusFault>(
        kStoragePowerAddress, state.locked_funds, 0));
    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    auto slashed{rewardForConsensusSlashReport(age, balance)};
    OUTCOME_TRY(runtime.sendFunds(runtime.getImmediateCaller(), slashed));
    OUTCOME_TRY(runtime.deleteActor());
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(WithdrawBalance) {
    VM_ASSERT(params.amount >= 0);
    OUTCOME_TRY(state, loadState(runtime));
    OUTCOME_TRY(runtime.validateImmediateCallerIs(state.info.owner));
    OUTCOME_TRY(new_vest, unlockVestedFunds(state, runtime.getCurrentEpoch()));
    OUTCOME_TRY(runtime.commitState(state));
    OUTCOME_TRY(balance, runtime.getCurrentBalance());
    auto amount{std::min(params.amount, getAvailableBalance(state, balance))};
    VM_ASSERT(amount <= balance);
    OUTCOME_TRY(runtime.sendFunds(state.info.owner, amount));
    OUTCOME_TRY(notifyPledgeChanged(runtime, -new_vest));
    OUTCOME_TRYA(balance, runtime.getCurrentBalance());
    OUTCOME_TRY(assertBalanceInvariants(state, balance));
    return outcome::success();
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
      exportMethod<AddLockedFund>(),
      exportMethod<ReportConsensusFault>(),
      exportMethod<WithdrawBalance>(),
  };
}  // namespace fc::vm::actor::builtin::miner
