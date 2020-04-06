/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"

#include "adt/uvarint_key.hpp"
#include "power/impl/power_table_hamt.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using power::PowerTableHamt;
  using primitives::ChainEpoch;
  using ChainEpochKeyer = adt::UvarintKeyer;

  StoragePowerActor::StoragePowerActor(std::shared_ptr<IpfsDatastore> datastore,
                                       StoragePowerActorState state)
      : datastore_{std::move(datastore)},
        state_{std::move(state)},
        cron_event_queue_(std::make_shared<Multimap>(
            datastore_, state_.cron_event_queue_cid)) {
    state_.load(datastore_);
  }

  outcome::result<StoragePowerActorState> StoragePowerActor::createEmptyState(
      std::shared_ptr<IpfsDatastore> datastore) {
    // create empty HAMT root CID
    Hamt empty_hamt(datastore);
    OUTCOME_TRY(cidEmpty, empty_hamt.flush());
    return StoragePowerActorState{
        0, 0, cidEmpty, cidEmpty, cidEmpty, cidEmpty, 0};
  }

  void StoragePowerActor::setState(const StoragePowerActorState &state) {
    state_ = state;
    cron_event_queue_ =
        std::make_shared<Multimap>(datastore_, state_.cron_event_queue_cid);
  }

  fc::outcome::result<StoragePowerActorState> StoragePowerActor::flushState() {
    OUTCOME_TRY(new_cron_event_queue_cid, cron_event_queue_->flush());
    state_.cron_event_queue_cid = new_cron_event_queue_cid;

    OUTCOME_TRY(state_.flush());

    return state_;
  }

  outcome::result<void> StoragePowerActor::addMiner(const Address &miner_addr) {
    OUTCOME_TRY(check, hasClaim(miner_addr));
    if (check) {
      return VMExitCode::STORAGE_POWER_ACTOR_ALREADY_EXISTS;
    }

    OUTCOME_TRY(state_.escrow.set(miner_addr, 0));
    OUTCOME_TRY(state_.claims.set(miner_addr, {0, 0}));

    state_.miner_count++;

    return outcome::success();
  }

  outcome::result<TokenAmount> StoragePowerActor::deleteMiner(
      const Address &miner_addr) {
    OUTCOME_TRY(assertHasEscrow(miner_addr));

    OUTCOME_TRY(claim, state_.claims.get(miner_addr));
    if (claim.power > Power{0})
      return outcome::failure(VMExitCode::STORAGE_POWER_FORBIDDEN);

    OUTCOME_TRY(state_.claims.remove(miner_addr));
    OUTCOME_TRY(old_balance, state_.escrow.get(miner_addr));
    OUTCOME_TRY(state_.escrow.remove(miner_addr));
    OUTCOME_TRY(contains, state_.po_st_detected_fault_miners.has(miner_addr));
    if (contains) {
      OUTCOME_TRY(state_.po_st_detected_fault_miners.remove(miner_addr));
    }

    state_.miner_count--;

    return std::move(old_balance);
  }

  outcome::result<bool> StoragePowerActor::hasMiner(
      const Address &miner_addr) const {
    return state_.escrow.has(miner_addr);
  }

  outcome::result<TokenAmount> StoragePowerActor::getMinerBalance(
      const Address &miner) const {
    OUTCOME_TRY(assertHasEscrow(miner));
    return state_.escrow.get(miner);
  }

  outcome::result<void> StoragePowerActor::setMinerBalance(
      const Address &miner, const TokenAmount &balance) {
    OUTCOME_TRY(assertHasEscrow(miner));
    return state_.escrow.set(miner, balance);
  }

  outcome::result<void> StoragePowerActor::addMinerBalance(
      const Address &miner, const TokenAmount &amount) {
    OUTCOME_TRY(assertHasEscrow(miner));
    return state_.escrow.add(miner, amount);
  }

  outcome::result<TokenAmount> StoragePowerActor::subtractMinerBalance(
      const Address &miner,
      const TokenAmount &amount,
      const TokenAmount &balance_floor) {
    OUTCOME_TRY(assertHasEscrow(miner));
    return state_.escrow.subtractWithMin(miner, amount, balance_floor);
  }

  outcome::result<void> StoragePowerActor::setClaim(const Address &miner,
                                                    const Claim &claim) {
    OUTCOME_TRY(assertHasClaim(miner));
    OUTCOME_TRY(state_.claims.set(miner, claim));
    return outcome::success();
  }

  outcome::result<bool> StoragePowerActor::hasClaim(
      const Address &miner) const {
    return state_.claims.has(miner);
  }

  outcome::result<Claim> StoragePowerActor::getClaim(const Address &miner) {
    return assertHasClaim(miner);
  }

  outcome::result<void> StoragePowerActor::deleteClaim(const Address &miner) {
    OUTCOME_TRY(assertHasClaim(miner));
    OUTCOME_TRY(state_.claims.remove(miner));
    return outcome::success();
  }

  outcome::result<void> StoragePowerActor::addToClaim(
      const Address &miner, const Power &power, const TokenAmount &pledge) {
    OUTCOME_TRY(claim, assertHasClaim(miner));
    claim.power += power;
    claim.pledge += pledge;
    OUTCOME_TRY(state_.claims.set(miner, claim));
    return outcome::success();
  }

  outcome::result<std::vector<Claim>> StoragePowerActor::getClaims() const {
    std::vector<Claim> all_claims;
    OUTCOME_TRY(state_.claims.visit(
        [&](auto &, auto &claim) -> fc::outcome::result<void> {
          all_claims.push_back(claim);
          return fc::outcome::success();
        }));
    return all_claims;
  };

  outcome::result<void> StoragePowerActor::appendCronEvent(
      const ChainEpoch &epoch, const CronEvent &event) {
    OUTCOME_TRY(
        cron_event_queue_->addCbor(ChainEpochKeyer::encode(epoch), event));
    return outcome::success();
  }

  outcome::result<std::vector<CronEvent>> StoragePowerActor::getCronEvents(
      const ChainEpoch &epoch) const {
    std::vector<CronEvent> events;
    Multimap::Visitor events_visitor{
        [&events](auto v) -> fc::outcome::result<void> {
          OUTCOME_TRY(event, codec::cbor::decode<CronEvent>(v));
          events.push_back(event);
          return fc::outcome::success();
        }};
    OUTCOME_TRY(cron_event_queue_->visit(ChainEpochKeyer::encode(epoch),
                                         events_visitor));
    return events;
  }

  outcome::result<void> StoragePowerActor::clearCronEvents(
      const ChainEpoch &epoch) {
    OUTCOME_TRY(cron_event_queue_->removeAll(ChainEpochKeyer::encode(epoch)));
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
    if (state_.num_miners_meeting_min_power > 0) {
      return false;
    }

    // else if none do, check whether in MIN_MINER_SIZE_TARG miners
    if (state_.miner_count <= kConsensusMinerMinMiners) {
      // miner should pass
      return true;
    }

    std::vector<Power> miner_sizes;
    OUTCOME_TRY(state_.claims.visit(
        [&](auto &address, auto &) -> fc::outcome::result<void> {
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
    OUTCOME_TRY(state_.po_st_detected_fault_miners.set(miner_addr, {}));
    return outcome::success();
  }

  outcome::result<bool> StoragePowerActor::hasFaultMiner(
      const Address &miner_addr) const {
    OUTCOME_TRY(assertHasClaim(miner_addr));
    return state_.po_st_detected_fault_miners.has(miner_addr);
  }

  outcome::result<void> StoragePowerActor::deleteFaultMiner(
      const Address &miner_addr) {
    return state_.po_st_detected_fault_miners.remove(miner_addr);
  }

  outcome::result<std::vector<Address>> StoragePowerActor::getFaultMiners()
      const {
    std::vector<Address> all_miners;
    OUTCOME_TRY(
        state_.po_st_detected_fault_miners.visit([&](auto &address, auto) {
          all_miners.push_back(address);
          return outcome::success();
        }));
    return all_miners;
  };

  outcome::result<std::vector<Address>> StoragePowerActor::getMiners() const {
    std::vector<Address> all_miners;
    OUTCOME_TRY(state_.claims.visit(
        [&](auto &address, auto &) -> fc::outcome::result<void> {
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
    OUTCOME_TRY(is_fault, state_.po_st_detected_fault_miners.has(address));
    return is_fault ? 0 : claim.power;
  }

  outcome::result<Power> StoragePowerActor::getTotalNetworkPower() const {
    return state_.total_network_power;
  }

  outcome::result<Claim> StoragePowerActor::assertHasClaim(
      const Address &address) const {
    OUTCOME_TRY(claim, state_.claims.tryGet(address));
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
