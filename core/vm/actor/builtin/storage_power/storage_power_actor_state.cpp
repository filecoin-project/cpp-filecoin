/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"

#include "power/impl/power_table_hamt.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using adt::Multimap;
  using power::PowerTableHamt;
  using primitives::ChainEpoch;

  outcome::result<StoragePowerActorState> StoragePowerActor::createEmptyState(
      std::shared_ptr<IpfsDatastore> datastore) {
    StoragePowerActorState state;
    state.load(datastore);
    state.total_network_power = 0;
    state.miner_count = 0;
    state.num_miners_meeting_min_power = 0;
    return std::move(state);
  }

  outcome::result<void> StoragePowerActor::addMiner(const Address &miner_addr) {
    OUTCOME_TRY(check, hasClaim(miner_addr));
    if (check) {
      return VMExitCode::STORAGE_POWER_ACTOR_ALREADY_EXISTS;
    }

    OUTCOME_TRY(escrow.set(miner_addr, 0));
    OUTCOME_TRY(claims.set(miner_addr, {0, 0}));

    miner_count++;

    return outcome::success();
  }

  outcome::result<TokenAmount> StoragePowerActor::deleteMiner(
      const Address &miner_addr) {
    OUTCOME_TRY(assertHasEscrow(miner_addr));

    OUTCOME_TRY(claim, claims.get(miner_addr));
    if (claim.power > Power{0})
      return outcome::failure(VMExitCode::STORAGE_POWER_FORBIDDEN);

    OUTCOME_TRY(claims.remove(miner_addr));
    OUTCOME_TRY(old_balance, escrow.get(miner_addr));
    OUTCOME_TRY(escrow.remove(miner_addr));
    OUTCOME_TRY(contains, po_st_detected_fault_miners.has(miner_addr));
    if (contains) {
      OUTCOME_TRY(po_st_detected_fault_miners.remove(miner_addr));
    }

    miner_count--;

    return std::move(old_balance);
  }

  outcome::result<bool> StoragePowerActor::hasMiner(
      const Address &miner_addr) const {
    return escrow.has(miner_addr);
  }

  outcome::result<TokenAmount> StoragePowerActor::getMinerBalance(
      const Address &miner) const {
    OUTCOME_TRY(assertHasEscrow(miner));
    return escrow.get(miner);
  }

  outcome::result<void> StoragePowerActor::setMinerBalance(
      const Address &miner, const TokenAmount &balance) {
    OUTCOME_TRY(assertHasEscrow(miner));
    return escrow.set(miner, balance);
  }

  outcome::result<void> StoragePowerActor::addMinerBalance(
      const Address &miner, const TokenAmount &amount) {
    OUTCOME_TRY(assertHasEscrow(miner));
    return escrow.add(miner, amount);
  }

  outcome::result<TokenAmount> StoragePowerActor::subtractMinerBalance(
      const Address &miner,
      const TokenAmount &amount,
      const TokenAmount &balance_floor) {
    OUTCOME_TRY(assertHasEscrow(miner));
    return escrow.subtractWithMin(miner, amount, balance_floor);
  }

  outcome::result<void> StoragePowerActor::setClaim(const Address &miner,
                                                    const Claim &claim) {
    OUTCOME_TRY(assertHasClaim(miner));
    OUTCOME_TRY(claims.set(miner, claim));
    return outcome::success();
  }

  outcome::result<bool> StoragePowerActor::hasClaim(
      const Address &miner) const {
    return claims.has(miner);
  }

  outcome::result<Claim> StoragePowerActor::getClaim(const Address &miner) {
    return assertHasClaim(miner);
  }

  outcome::result<void> StoragePowerActor::deleteClaim(const Address &miner) {
    OUTCOME_TRY(assertHasClaim(miner));
    OUTCOME_TRY(claims.remove(miner));
    return outcome::success();
  }

  outcome::result<void> StoragePowerActor::addToClaim(
      const Address &miner, const Power &power, const TokenAmount &pledge) {
    OUTCOME_TRY(claim, assertHasClaim(miner));
    claim.power += power;
    claim.pledge += pledge;
    OUTCOME_TRY(claims.set(miner, claim));
    return outcome::success();
  }

  outcome::result<std::vector<Claim>> StoragePowerActor::getClaims() const {
    std::vector<Claim> all_claims;
    OUTCOME_TRY(
        claims.visit([&](auto &, auto &claim) -> fc::outcome::result<void> {
          all_claims.push_back(claim);
          return fc::outcome::success();
        }));
    return all_claims;
  };

  outcome::result<void> StoragePowerActor::appendCronEvent(
      const ChainEpoch &epoch, const CronEvent &event) {
    return Multimap::append(cron_event_queue, epoch, event);
  }

  outcome::result<void> f(const CronEvent &event) {
    return outcome::success();
  }

  outcome::result<std::vector<CronEvent>> StoragePowerActor::getCronEvents(
      const ChainEpoch &epoch) const {
    std::vector<CronEvent> events;
    OUTCOME_TRY(Multimap::visit(cron_event_queue, epoch, {[&](auto &event) {
                                  events.push_back(event);
                                  return outcome::success();
                                }}));
    return events;
  }

  outcome::result<void> StoragePowerActor::clearCronEvents(
      const ChainEpoch &epoch) {
    OUTCOME_TRY(cron_event_queue.remove(epoch));
    return outcome::success();
  }

  outcome::result<bool>
  StoragePowerActor::minerNominalPowerMeetsConsensusMinimum(
      const power::Power &miner_power) {
    // if miner is larger than min power requirement, we're set
    if (miner_power >= kConsensusMinerMinPower) {
      return true;
    }

    // otherwise, if another miner meets min power requirement, return false
    if (num_miners_meeting_min_power > 0) {
      return false;
    }

    // else if none do, check whether in MIN_MINER_SIZE_TARG miners
    if (miner_count <= kConsensusMinerMinMiners) {
      // miner should pass
      return true;
    }

    std::vector<Power> miner_sizes;
    OUTCOME_TRY(
        claims.visit([&](auto &address, auto &) -> fc::outcome::result<void> {
          OUTCOME_TRY(nominal_power, this->computeNominalPower(address));
          miner_sizes.push_back(nominal_power);
          return fc::outcome::success();
        }));
    std::sort(miner_sizes.begin(),
              miner_sizes.end(),
              [](const Power &l, const Power &r) { return l > r; });
    return miner_power >= miner_sizes[kConsensusMinerMinMiners - 1];
  }

  outcome::result<void> StoragePowerActor::addFaultMiner(
      const Address &miner_addr) {
    OUTCOME_TRY(assertHasClaim(miner_addr));
    OUTCOME_TRY(po_st_detected_fault_miners.set(miner_addr, {}));
    return outcome::success();
  }

  outcome::result<bool> StoragePowerActor::hasFaultMiner(
      const Address &miner_addr) const {
    OUTCOME_TRY(assertHasClaim(miner_addr));
    return po_st_detected_fault_miners.has(miner_addr);
  }

  outcome::result<void> StoragePowerActor::deleteFaultMiner(
      const Address &miner_addr) {
    return po_st_detected_fault_miners.remove(miner_addr);
  }

  outcome::result<std::vector<Address>> StoragePowerActor::getFaultMiners()
      const {
    std::vector<Address> all_miners;
    OUTCOME_TRY(po_st_detected_fault_miners.visit([&](auto &address, auto) {
      all_miners.push_back(address);
      return outcome::success();
    }));
    return all_miners;
  };

  outcome::result<std::vector<Address>> StoragePowerActor::getMiners() const {
    std::vector<Address> all_miners;
    OUTCOME_TRY(
        claims.visit([&](auto &address, auto &) -> fc::outcome::result<void> {
          OUTCOME_TRY(nominal_power, this->computeNominalPower(address));
          if (nominal_power > 0) {
            all_miners.push_back(address);
          }
          return fc::outcome::success();
        }));
    return all_miners;
  };

  outcome::result<Power> StoragePowerActor::computeNominalPower(
      const Address &address) const {
    OUTCOME_TRY(claim, assertHasClaim(address));
    OUTCOME_TRY(is_fault, po_st_detected_fault_miners.has(address));
    return is_fault ? 0 : claim.power;
  }

  outcome::result<Power> StoragePowerActor::getTotalNetworkPower() const {
    return total_network_power;
  }

  outcome::result<Claim> StoragePowerActor::assertHasClaim(
      const Address &address) const {
    OUTCOME_TRY(claim, claims.tryGet(address));
    if (!claim) {
      return VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT;
    }
    return *claim;
  }

  outcome::result<void> StoragePowerActor::assertHasEscrow(
      const Address &address) const {
    OUTCOME_TRY(has, hasMiner(address));
    if (!has) {
      return VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT;
    }
    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::storage_power
