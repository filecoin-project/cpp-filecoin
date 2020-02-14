/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"

#include "power/impl/power_table_hamt.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/chain_epoch/chain_epoch_codec.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/indices/indices.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using power::PowerTableHamt;
  using primitives::ChainEpoch;
  using primitives::address::Address;
  using primitives::address::decodeFromByteString;
  using primitives::address::encodeToByteString;

  StoragePowerActorState::StoragePowerActorState(
      std::shared_ptr<Indices> indices,
      std::shared_ptr<crypto::randomness::RandomnessProvider>
          randomness_provider,
      std::shared_ptr<Hamt> escrow_table,
      std::shared_ptr<Multimap> cron_event_queue,
      std::shared_ptr<Hamt> po_st_detected_fault_miners,
      std::shared_ptr<Hamt> claims)
      : indices_(std::move(indices)),
        randomness_provider_(std::move(randomness_provider)),
        total_network_power_(0),
        miner_count_(0),
        escrow_table_(std::move(escrow_table)),
        cron_event_queue_(std::move(cron_event_queue)),
        po_st_detected_fault_miners_(std::move(po_st_detected_fault_miners)),
        claims_(std::move(claims)),
        num_miners_meeting_min_power_(0) {}

  outcome::result<void> StoragePowerActorState::addMiner(
      const primitives::address::Address &miner_addr) {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner_addr)));
    if (check) {
      return VMExitCode::STORAGE_POWER_ACTOR_ALREADY_EXISTS;
    }

    OUTCOME_TRY(claims_->setCbor(encodeToByteString(miner_addr), Claim{0, 0}));
    OUTCOME_TRY(
        escrow_table_->setCbor(encodeToByteString(miner_addr), TokenAmount{0}));
    OUTCOME_TRY(claims_cid, claims_->flush());
    claims_cid_ = claims_cid;
    OUTCOME_TRY(escrow_table_cid, claims_->flush());
    escrow_table_cid_ = escrow_table_cid;

    return outcome::success();
  }

  outcome::result<void> StoragePowerActorState::deleteMiner(
      const primitives::address::Address &miner_addr) {
    OUTCOME_TRY(balance, getMinerBalance(miner_addr));

    if (balance > TokenAmount{0})
      return VMExitCode::STORAGE_POWER_DELETION_ERROR;

    std::string encoded_miner_addr = encodeToByteString(miner_addr);
    OUTCOME_TRY(claim, claims_->getCbor<Claim>(encoded_miner_addr));
    if (claim.power > Power{0}) return VMExitCode::STORAGE_POWER_DELETION_ERROR;

    OUTCOME_TRY(claims_->remove(encoded_miner_addr));
    OUTCOME_TRY(claims_cid, claims_->flush());
    claims_cid_ = claims_cid;

    OUTCOME_TRY(escrow_table_->remove(encoded_miner_addr));
    OUTCOME_TRY(escrow_table_cid, claims_->flush());
    escrow_table_cid_ = escrow_table_cid;

    OUTCOME_TRY(contains,
                po_st_detected_fault_miners_->contains(encoded_miner_addr));
    if (contains) {
      OUTCOME_TRY(po_st_detected_fault_miners_->remove(encoded_miner_addr));

      OUTCOME_TRY(po_st_detected_fault_miners_cid,
                  po_st_detected_fault_miners_->flush());
      po_st_detected_fault_miners_cid_ = po_st_detected_fault_miners_cid;
    }

    return outcome::success();
  }

  outcome::result<std::vector<primitives::address::Address>>
  StoragePowerActorState::selectMinersToSurprise(
      size_t challenge_count,
      const crypto::randomness::Randomness &randomness) {
    OUTCOME_TRY(all_miners, getMiners());
    if (challenge_count > all_miners.size())
      return VMExitCode::STORAGE_POWER_ACTOR_OUT_OF_BOUND;

    if (challenge_count == all_miners.size()) return std::move(all_miners);

    std::vector<primitives::address::Address> selected_miners;
    for (size_t chall = 0; chall < challenge_count; chall++) {
      int miner_index =
          randomness_provider_->randomInt(randomness, chall, all_miners.size());
      auto potential_challengee = all_miners[miner_index];

      // skip dups
      while (std::find(selected_miners.cbegin(),
                       selected_miners.cend(),
                       potential_challengee)
             != selected_miners.cend()) {
        miner_index = randomness_provider_->randomInt(
            randomness,
            chall,
            all_miners.size());  // TODO (a.chernyshov) from specs-actor fix
                                 // this, it's a constant value
        potential_challengee = all_miners[miner_index];
      }

      selected_miners.push_back(potential_challengee);
    }

    return selected_miners;
  }

  outcome::result<TokenAmount> StoragePowerActorState::getMinerBalance(
      const Address &miner) const {
    OUTCOME_TRY(check, escrow_table_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND);

    return escrow_table_->getCbor<TokenAmount>(encodeToByteString(miner));
  }

  outcome::result<void> StoragePowerActorState::setMinerBalance(
      const Address &miner, const TokenAmount &balance) {
    OUTCOME_TRY(check, escrow_table_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND);

    OUTCOME_TRY(escrow_table_->setCbor(encodeToByteString(miner), balance));
    OUTCOME_TRY(escrow_table_cid, escrow_table_->flush());
    escrow_table_cid_ = escrow_table_cid;
    return outcome::success();
  }

  outcome::result<void> StoragePowerActorState::addMinerBalance(
      const Address &miner, const TokenAmount &amount) {
    OUTCOME_TRY(check, escrow_table_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND);

    OUTCOME_TRY(balance,
                escrow_table_->getCbor<TokenAmount>(encodeToByteString(miner)));
    OUTCOME_TRY(
        escrow_table_->setCbor(encodeToByteString(miner), balance + amount));
    OUTCOME_TRY(escrow_table_cid, escrow_table_->flush());
    escrow_table_cid_ = escrow_table_cid;
    return outcome::success();
  }

  outcome::result<TokenAmount> StoragePowerActorState::subtractMinerBalance(
      const Address &miner,
      const TokenAmount &amount,
      const TokenAmount &balance_floor) {
    OUTCOME_TRY(check, escrow_table_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND);

    OUTCOME_TRY(balance,
                escrow_table_->getCbor<TokenAmount>(encodeToByteString(miner)));
    TokenAmount available =
        balance < balance_floor ? 0 : balance - balance_floor;
    TokenAmount sub = available < amount ? available : amount;
    OUTCOME_TRY(
        escrow_table_->setCbor(encodeToByteString(miner), balance - sub));
    OUTCOME_TRY(escrow_table_cid, escrow_table_->flush());
    escrow_table_cid_ = escrow_table_cid;
    return sub;
  }

  outcome::result<void> StoragePowerActorState::setClaim(const Address &miner,
                                                         const Claim &claim) {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND);
    OUTCOME_TRY(claims_->setCbor(encodeToByteString(miner), claim));
    OUTCOME_TRY(claims_cid, claims_->flush());
    claims_cid_ = claims_cid;
    return outcome::success();
  }

  outcome::result<Claim> StoragePowerActorState::getClaim(
      const Address &miner) {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND);
    return claims_->getCbor<Claim>(encodeToByteString(miner));
  }

  outcome::result<void> StoragePowerActorState::deleteClaim(
      const Address &miner) {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND);
    OUTCOME_TRY(claims_->remove(encodeToByteString(miner)));
    OUTCOME_TRY(claims_cid, claims_->flush());
    claims_cid_ = claims_cid;
    return outcome::success();
  }

  outcome::result<void> StoragePowerActorState::addToClaim(
      const Address &miner, const Power &power, const TokenAmount &pledge) {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND);
    OUTCOME_TRY(claim, claims_->getCbor<Claim>(encodeToByteString(miner)));
    claim.power += power;
    claim.pledge += pledge;
    OUTCOME_TRY(claims_->setCbor(encodeToByteString(miner), claim));
    OUTCOME_TRY(claims_cid, claims_->flush());
    claims_cid_ = claims_cid;
    return outcome::success();
  }

  outcome::result<void> StoragePowerActorState::appendCronEvent(
      const ChainEpoch &epoch, const CronEvent &event) {
    OUTCOME_TRY(cron_event_queue_->addCbor(
        primitives::chain_epoch::encodeToByteString(epoch), event));
    OUTCOME_TRY(cron_event_queue_cid, cron_event_queue_->flush());
    cron_event_queue_cid_ = cron_event_queue_cid;
    return outcome::success();
  }

  outcome::result<bool>
  StoragePowerActorState::minerNominalPowerMeetsConsensusMinimum(
      const power::Power &miner_power) {
    // if miner is larger than min power requirement, we're set
    if (miner_power >= kConsensusMinerMinPower) {
      return true;
    }

    // otherwise, if another miner meets min power requirement, return false
    if (num_miners_meeting_min_power_ > 0) {
      return false;
    }

    // else if none do, check whether in MIN_MINER_SIZE_TARG miners
    if (miner_count_ <= kConsensusMinerMinMiners) {
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

  outcome::result<void> StoragePowerActorState::addFaultMiner(
      const primitives::address::Address &miner_addr) {
    // check that miner exist
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(miner_addr)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND);
    return po_st_detected_fault_miners_->set(encodeToByteString(miner_addr),
                                             {});
    OUTCOME_TRY(po_st_detected_fault_miners_cid,
                po_st_detected_fault_miners_->flush());
    po_st_detected_fault_miners_cid_ = po_st_detected_fault_miners_cid;
    return outcome::success();
  }

  outcome::result<void> StoragePowerActorState::deleteFaultMiner(
      const primitives::address::Address &miner_addr) {
    return po_st_detected_fault_miners_->remove(encodeToByteString(miner_addr));
  }

  outcome::result<std::vector<primitives::address::Address>>
  StoragePowerActorState::getMiners() const {
    std::vector<Address> all_miners;
    Hamt::Visitor miner_power_visitor{
        [this, &all_miners](auto k, auto v) -> fc::outcome::result<void> {
          OUTCOME_TRY(address, decodeFromByteString(k));
          OUTCOME_TRY(nominal_power, this->computeNominalPower(address));
          if (nominal_power > Power{0}) all_miners.push_back(address);
          return fc::outcome::success();
        }};
    OUTCOME_TRY(claims_->visit(miner_power_visitor));
    return std::move(all_miners);
  };

  outcome::result<Power> StoragePowerActorState::computeNominalPower(
      const Address &address) const {
    OUTCOME_TRY(check, claims_->contains(encodeToByteString(address)));
    if (!check)
      return outcome::failure(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND);
    OUTCOME_TRY(claimed, claims_->getCbor<Claim>(encodeToByteString(address)));
    Power nominal_power = claimed.power;
    std::string encoded_miner_addr = encodeToByteString(address);
    OUTCOME_TRY(is_fault,
                po_st_detected_fault_miners_->contains(encoded_miner_addr));
    if (is_fault) nominal_power = 0;
    return nominal_power;
  }

}  // namespace fc::vm::actor::builtin::storage_power
