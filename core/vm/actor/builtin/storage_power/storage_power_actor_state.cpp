/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"

#include "adt/address_key.hpp"
#include "adt/empty_value.hpp"
#include "adt/uvarint_key.hpp"
#include "power/impl/power_table_hamt.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using power::PowerTableHamt;
  using primitives::ChainEpoch;
  using primitives::address::Address;
  using ChainEpochKeyer = adt::UvarintKeyer;
  using adt::AddressKeyer;

  StoragePowerActor::StoragePowerActor(std::shared_ptr<IpfsDatastore> datastore,
                                       StoragePowerActorState state)
      : datastore_{std::move(datastore)},
        state_{std::move(state)},
        escrow_table_(std::make_shared<BalanceTableHamt>(
            datastore_, state_.escrow_table_cid)),
        cron_event_queue_(std::make_shared<Multimap>(
            datastore_, state_.cron_event_queue_cid)),
        po_st_detected_fault_miners_(std::make_shared<Hamt>(
            datastore_, state_.po_st_detected_fault_miners_cid)),
        claims_(std::make_shared<Hamt>(datastore_, state_.claims_cid)) {}

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
    escrow_table_ =
        std::make_shared<BalanceTableHamt>(datastore_, state_.escrow_table_cid);
    cron_event_queue_ =
        std::make_shared<Multimap>(datastore_, state_.cron_event_queue_cid);
    po_st_detected_fault_miners_ = std::make_shared<Hamt>(
        datastore_, state_.po_st_detected_fault_miners_cid);
    claims_ = std::make_shared<Hamt>(datastore_, state_.claims_cid);
  }

  fc::outcome::result<StoragePowerActorState> StoragePowerActor::flushState() {
    state_.escrow_table_cid = escrow_table_->root;

    OUTCOME_TRY(new_po_st_detected_fault_miners_cid,
                po_st_detected_fault_miners_->flush());
    state_.po_st_detected_fault_miners_cid =
        new_po_st_detected_fault_miners_cid;

    OUTCOME_TRY(new_cron_event_queue_cid, cron_event_queue_->flush());
    state_.cron_event_queue_cid = new_cron_event_queue_cid;

    OUTCOME_TRY(new_claims_cid, claims_->flush());
    state_.claims_cid = new_claims_cid;

    return state_;
  }

  outcome::result<void> StoragePowerActor::addMiner(const Address &miner_addr) {
    OUTCOME_TRY(check, hasClaim(miner_addr));
    if (check) {
      return VMExitCode::STORAGE_POWER_ACTOR_ALREADY_EXISTS;
    }

    OUTCOME_TRY(escrow_table_->set(miner_addr, TokenAmount{0}));
    OUTCOME_TRY(
        claims_->setCbor(AddressKeyer::encode(miner_addr), Claim{0, 0}));

    state_.miner_count++;

    return outcome::success();
  }

  outcome::result<TokenAmount> StoragePowerActor::deleteMiner(
      const Address &miner_addr) {
    OUTCOME_TRY(assertHasEscrow(miner_addr));

    std::string encoded_miner_addr = AddressKeyer::encode(miner_addr);
    OUTCOME_TRY(claim, claims_->getCbor<Claim>(encoded_miner_addr));
    if (claim.power > Power{0})
      return outcome::failure(VMExitCode::STORAGE_POWER_FORBIDDEN);

    OUTCOME_TRY(claims_->remove(encoded_miner_addr));
    OUTCOME_TRY(old_balance, escrow_table_->remove(miner_addr));
    OUTCOME_TRY(contains,
                po_st_detected_fault_miners_->contains(encoded_miner_addr));
    if (contains) {
      OUTCOME_TRY(po_st_detected_fault_miners_->remove(encoded_miner_addr));
    }

    state_.miner_count--;

    return std::move(old_balance);
  }

  outcome::result<bool> StoragePowerActor::hasMiner(
      const Address &miner_addr) const {
    return escrow_table_->has(miner_addr);
  }

  outcome::result<TokenAmount> StoragePowerActor::getMinerBalance(
      const Address &miner) const {
    OUTCOME_TRY(assertHasEscrow(miner));
    return escrow_table_->get(miner);
  }

  outcome::result<void> StoragePowerActor::setMinerBalance(
      const Address &miner, const TokenAmount &balance) {
    OUTCOME_TRY(assertHasEscrow(miner));
    return escrow_table_->set(miner, balance);
  }

  outcome::result<void> StoragePowerActor::addMinerBalance(
      const Address &miner, const TokenAmount &amount) {
    OUTCOME_TRY(assertHasEscrow(miner));
    return escrow_table_->add(miner, amount);
  }

  outcome::result<TokenAmount> StoragePowerActor::subtractMinerBalance(
      const Address &miner,
      const TokenAmount &amount,
      const TokenAmount &balance_floor) {
    OUTCOME_TRY(assertHasEscrow(miner));
    return escrow_table_->subtractWithMinimum(miner, amount, balance_floor);
  }

  outcome::result<void> StoragePowerActor::setClaim(const Address &miner,
                                                    const Claim &claim) {
    OUTCOME_TRY(assertHasClaim(miner));
    OUTCOME_TRY(claims_->setCbor(AddressKeyer::encode(miner), claim));
    return outcome::success();
  }

  outcome::result<bool> StoragePowerActor::hasClaim(
      const Address &miner) const {
    return claims_->contains(AddressKeyer::encode(miner));
  }

  outcome::result<Claim> StoragePowerActor::getClaim(const Address &miner) {
    return assertHasClaim(miner);
  }

  outcome::result<void> StoragePowerActor::deleteClaim(const Address &miner) {
    OUTCOME_TRY(assertHasClaim(miner));
    OUTCOME_TRY(claims_->remove(AddressKeyer::encode(miner)));
    return outcome::success();
  }

  outcome::result<void> StoragePowerActor::addToClaim(
      const Address &miner, const Power &power, const TokenAmount &pledge) {
    OUTCOME_TRY(claim, assertHasClaim(miner));
    claim.power += power;
    claim.pledge += pledge;
    OUTCOME_TRY(claims_->setCbor(AddressKeyer::encode(miner), claim));
    return outcome::success();
  }

  outcome::result<std::vector<Claim>> StoragePowerActor::getClaims() const {
    std::vector<Claim> all_claims;
    Hamt::Visitor all_claims_visitor{
        [&all_claims](auto k, auto v) -> fc::outcome::result<void> {
          OUTCOME_TRY(claim, codec::cbor::decode<Claim>(v));
          all_claims.push_back(claim);
          return fc::outcome::success();
        }};
    OUTCOME_TRY(po_st_detected_fault_miners_->visit(all_claims_visitor));
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
    Hamt::Visitor miner_power_visitor{
        [this, &miner_sizes](auto k, auto v) -> fc::outcome::result<void> {
          OUTCOME_TRY(address, AddressKeyer::decode(k));
          OUTCOME_TRY(nominal_power, this->computeNominalPower(address));
          miner_sizes.push_back(nominal_power);
          return fc::outcome::success();
        }};
    OUTCOME_TRY(claims_->visit(miner_power_visitor));
    std::sort(miner_sizes.begin(),
              miner_sizes.end(),
              [](const Power &l, const Power &r) { return l > r; });
    return miner_power >= miner_sizes[kConsensusMinerMinMiners - 1];
  }

  outcome::result<void> StoragePowerActor::addFaultMiner(
      const Address &miner_addr) {
    // check that miner exist
    OUTCOME_TRY(assertHasClaim(miner_addr));
    // cbor empty value as empty list
    OUTCOME_TRY(po_st_detected_fault_miners_->setCbor(
        AddressKeyer::encode(miner_addr), adt::EmptyValue{}));
    return outcome::success();
  }

  outcome::result<bool> StoragePowerActor::hasFaultMiner(
      const Address &miner_addr) const {
    OUTCOME_TRY(assertHasClaim(miner_addr));
    OUTCOME_TRY(has_fault,
                po_st_detected_fault_miners_->contains(
                    AddressKeyer::encode(miner_addr)));
    return has_fault;
  }

  outcome::result<void> StoragePowerActor::deleteFaultMiner(
      const Address &miner_addr) {
    return po_st_detected_fault_miners_->remove(
        AddressKeyer::encode(miner_addr));
  }

  outcome::result<std::vector<Address>> StoragePowerActor::getFaultMiners()
      const {
    std::vector<Address> all_miners;
    Hamt::Visitor all_miners_visitor{
        [&all_miners](auto k, auto v) -> fc::outcome::result<void> {
          OUTCOME_TRY(address, AddressKeyer::decode(k));
          all_miners.push_back(address);
          return fc::outcome::success();
        }};
    OUTCOME_TRY(po_st_detected_fault_miners_->visit(all_miners_visitor));
    return all_miners;
  };

  outcome::result<std::vector<Address>> StoragePowerActor::getMiners() const {
    std::vector<Address> all_miners;
    Hamt::Visitor all_miners_visitor{
        [this, &all_miners](auto k, auto v) -> fc::outcome::result<void> {
          OUTCOME_TRY(address, AddressKeyer::decode(k));
          OUTCOME_TRY(nominal_power, this->computeNominalPower(address));
          if (nominal_power > Power{0}) all_miners.push_back(address);
          return fc::outcome::success();
        }};
    OUTCOME_TRY(claims_->visit(all_miners_visitor));
    return all_miners;
  };

  outcome::result<Power> StoragePowerActor::computeNominalPower(
      const Address &address) const {
    OUTCOME_TRY(claim, assertHasClaim(address));
    OUTCOME_TRY(
        is_fault,
        po_st_detected_fault_miners_->contains(AddressKeyer::encode(address)));
    return is_fault ? 0 : claim.power;
  }

  outcome::result<Power> StoragePowerActor::getTotalNetworkPower() const {
    return state_.total_network_power;
  }

  outcome::result<Claim> StoragePowerActor::assertHasClaim(
      const Address &address) const {
    OUTCOME_TRY(claim,
                claims_->tryGetCbor<Claim>(AddressKeyer::encode(address)));
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
