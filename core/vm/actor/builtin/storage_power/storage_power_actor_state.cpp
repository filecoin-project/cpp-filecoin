/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"

#include "power/impl/power_table_hamt.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/chain_epoch/chain_epoch_codec.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using power::PowerTableHamt;
  using primitives::ChainEpoch;
  using primitives::address::Address;
  using primitives::address::decodeFromByteString;
  using primitives::address::encodeToByteString;

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
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner_addr)));
    if (check) {
      return VMExitCode::STORAGE_POWER_ACTOR_ALREADY_EXISTS;
    }

    OUTCOME_TRY(escrow_table_->set(miner_addr, TokenAmount{0}));
    OUTCOME_TRY(claims_->setCbor(encodeToByteString(miner_addr), Claim{0, 0}));

    state_.miner_count++;

    return outcome::success();
  }

  outcome::result<TokenAmount> StoragePowerActor::deleteMiner(
      const Address &miner_addr) {
    OUTCOME_TRY(miner_exists, escrow_table_->has(miner_addr));
    if (!miner_exists) {
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    }

    std::string encoded_miner_addr = encodeToByteString(miner_addr);
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
    OUTCOME_TRY(check, escrow_table_->has(miner_addr));
    return check;
  }

  outcome::result<TokenAmount> StoragePowerActor::getMinerBalance(
      const Address &miner) const {
    OUTCOME_TRY(check, escrow_table_->has(miner));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    return escrow_table_->get(miner);
  }

  outcome::result<void> StoragePowerActor::setMinerBalance(
      const Address &miner, const TokenAmount &balance) {
    OUTCOME_TRY(check, escrow_table_->has(miner));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    OUTCOME_TRY(escrow_table_->set(miner, balance));
    return outcome::success();
  }

  outcome::result<void> StoragePowerActor::addMinerBalance(
      const Address &miner, const TokenAmount &amount) {
    OUTCOME_TRY(check, escrow_table_->has(miner));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    OUTCOME_TRY(escrow_table_->add(miner, amount));
    return outcome::success();
  }

  outcome::result<TokenAmount> StoragePowerActor::subtractMinerBalance(
      const Address &miner,
      const TokenAmount &amount,
      const TokenAmount &balance_floor) {
    OUTCOME_TRY(check, escrow_table_->has(miner));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    return escrow_table_->subtractWithMinimum(miner, amount, balance_floor);
  }

  outcome::result<void> StoragePowerActor::setClaim(const Address &miner,
                                                    const Claim &claim) {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    OUTCOME_TRY(claims_->setCbor(encodeToByteString(miner), claim));
    return outcome::success();
  }

  outcome::result<bool> StoragePowerActor::hasClaim(
      const Address &miner) const {
    OUTCOME_TRY(res, claims_->contains(encodeToByteString(miner)));
    return res;
  }

  outcome::result<Claim> StoragePowerActor::getClaim(const Address &miner) {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    return claims_->getCbor<Claim>(encodeToByteString(miner));
  }

  outcome::result<void> StoragePowerActor::deleteClaim(const Address &miner) {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    OUTCOME_TRY(claims_->remove(encodeToByteString(miner)));
    return outcome::success();
  }

  outcome::result<void> StoragePowerActor::addToClaim(
      const Address &miner, const Power &power, const TokenAmount &pledge) {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    OUTCOME_TRY(claim, claims_->getCbor<Claim>(encodeToByteString(miner)));
    claim.power += power;
    claim.pledge += pledge;
    OUTCOME_TRY(claims_->setCbor(encodeToByteString(miner), claim));
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
    OUTCOME_TRY(cron_event_queue_->addCbor(
        primitives::chain_epoch::encodeToByteString(epoch), event));
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
    OUTCOME_TRY(cron_event_queue_->visit(
        primitives::chain_epoch::encodeToByteString(epoch), events_visitor));
    return events;
  }

  outcome::result<void> StoragePowerActor::clearCronEvents(
      const ChainEpoch &epoch) {
    OUTCOME_TRY(cron_event_queue_->removeAll(
        primitives::chain_epoch::encodeToByteString(epoch)));
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
          OUTCOME_TRY(address, decodeFromByteString(k));
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
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner_addr)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    // cbor empty value as empty list
    OUTCOME_TRY(po_st_detected_fault_miners_->setCbor<std::vector<int>>(
        encodeToByteString(miner_addr), {}));
    return outcome::success();
  }

  outcome::result<bool> StoragePowerActor::hasFaultMiner(
      const Address &miner_addr) const {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner_addr)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    OUTCOME_TRY(
        has_fault,
        po_st_detected_fault_miners_->contains(encodeToByteString(miner_addr)));
    return has_fault;
  }

  outcome::result<void> StoragePowerActor::deleteFaultMiner(
      const Address &miner_addr) {
    return po_st_detected_fault_miners_->remove(encodeToByteString(miner_addr));
  }

  outcome::result<std::vector<Address>> StoragePowerActor::getFaultMiners()
      const {
    std::vector<Address> all_miners;
    Hamt::Visitor all_miners_visitor{
        [&all_miners](auto k, auto v) -> fc::outcome::result<void> {
          OUTCOME_TRY(address, decodeFromByteString(k));
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
          OUTCOME_TRY(address, decodeFromByteString(k));
          OUTCOME_TRY(nominal_power, this->computeNominalPower(address));
          if (nominal_power > Power{0}) all_miners.push_back(address);
          return fc::outcome::success();
        }};
    OUTCOME_TRY(claims_->visit(all_miners_visitor));
    return all_miners;
  };

  outcome::result<Power> StoragePowerActor::computeNominalPower(
      const Address &address) const {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(address)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT);
    OUTCOME_TRY(claimed, claims_->getCbor<Claim>(encodeToByteString(address)));
    Power nominal_power = claimed.power;
    std::string encoded_miner_addr = encodeToByteString(address);
    OUTCOME_TRY(is_fault,
                po_st_detected_fault_miners_->contains(encoded_miner_addr));
    if (is_fault) nominal_power = 0;
    return nominal_power;
  }

  outcome::result<Power> StoragePowerActor::getTotalNetworkPower() const {
    return state_.total_network_power;
  }

}  // namespace fc::vm::actor::builtin::storage_power
