/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"

#include "power/impl/power_table_hamt.hpp"
#include "power/impl/power_table_impl.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/indices/indices.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using power::PowerTableHamt;
  using primitives::address::encodeToByteString;

  StoragePowerActorState::StoragePowerActorState(
      std::shared_ptr<Indices> indices,
      std::shared_ptr<crypto::randomness::RandomnessProvider>
          randomness_provider,
      std::shared_ptr<power::PowerTable> escrow_table,
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
        num_miners_meeting_min_power_(0),
        power_table_(std::make_unique<power::PowerTableImpl>()),
        claimed_power_(std::make_unique<power::PowerTableImpl>()),
        nominal_power_(std::make_unique<power::PowerTableImpl>()) {}

  outcome::result<std::vector<primitives::address::Address>>
  StoragePowerActorState::selectMinersToSurprise(
      size_t challenge_count,
      const crypto::randomness::Randomness &randomness) {
    std::vector<primitives::address::Address> selected_miners;

    OUTCOME_TRY(size, power_table_->getSize());
    if (size < challenge_count) {
      return VMExitCode::STORAGE_POWER_ACTOR_OUT_OF_BOUND;
    }

    OUTCOME_TRY(all_miners, power_table_->getMiners());

    if (challenge_count == all_miners.size()) return std::move(all_miners);

    for (size_t chall = 0; chall < challenge_count; chall++) {
      int miner_index =
          randomness_provider_->randomInt(randomness, chall, all_miners.size());
      auto potential_challengee = all_miners[miner_index];

      while (std::find(selected_miners.cbegin(),
                       selected_miners.cend(),
                       potential_challengee)
             != selected_miners.cend()) {
        miner_index = randomness_provider_->randomInt(
            randomness, chall, all_miners.size());
        potential_challengee = all_miners[miner_index];
      }

      selected_miners.push_back(potential_challengee);
    }

    return selected_miners;
  }

  outcome::result<void> StoragePowerActorState::addClaimedPowerForSector(
      const primitives::address::Address &miner_addr,
      const SectorStorageWeightDesc &storage_weight_desc) {
    // TODO(artyom-yurin): [FIL-135] FROM SPEC: The function is located in the
    // indices module temporarily, until we find a better place for global
    // parameterization functions.
    power::Power sector_power =
        indices_->consensusPowerForStorageWeight(storage_weight_desc);

    OUTCOME_TRY(miner_power, claimed_power_->getMinerPower(miner_addr));

    OUTCOME_TRY(
        setClaimedPowerEntryInternal(miner_addr, miner_power + sector_power));

    return updatePowerEntriesFromClaimedPower(miner_addr);
  }

  outcome::result<void>
  StoragePowerActorState::deductClaimedPowerForSectorAssert(
      const primitives::address::Address &miner_addr,
      const SectorStorageWeightDesc &storage_weight_desc) {
    // TODO(artyom-yurin): [FIL-135] FROM SPEC: The function is located in the
    // indices module temporarily, until we find a better place for global
    // parameterization functions.
    power::Power sector_power =
        indices_->consensusPowerForStorageWeight(storage_weight_desc);

    OUTCOME_TRY(miner_power, claimed_power_->getMinerPower(miner_addr));

    OUTCOME_TRY(
        setClaimedPowerEntryInternal(miner_addr, miner_power - sector_power));

    return updatePowerEntriesFromClaimedPower(miner_addr);
  }

  outcome::result<void>
  StoragePowerActorState::updatePowerEntriesFromClaimedPower(
      const primitives::address::Address &miner_addr) {
    OUTCOME_TRY(claimed_power, claimed_power_->getMinerPower(miner_addr));

    power::Power nominal_power = claimed_power;
    OUTCOME_TRY(
        contains,
        po_st_detected_fault_miners_->contains(encodeToByteString(miner_addr)));
    if (contains) nominal_power = 0;
    OUTCOME_TRY(setNominalPowerEntry(miner_addr, nominal_power));

    // Compute actual (consensus) power, i.e., votes in leader election.
    power::Power power = nominal_power;
    OUTCOME_TRY(meetsConsensusMinimum,
                minerNominalPowerMeetsConsensusMinimum(nominal_power));
    if (!meetsConsensusMinimum) {
      power = 0;
    }

    // TODO(artyom-yurin): [FIL-136] FROM DOCS Decide effect of
    // undercollateralization on (consensus) power.

    return setPowerEntryInternal(miner_addr, power);
  }

  outcome::result<void> StoragePowerActorState::setNominalPowerEntry(
      const primitives::address::Address &miner_addr,
      const power::Power &updated_nominal_power) {
    OUTCOME_TRY(prev_miner_nominal_power,
                nominal_power_->getMinerPower(miner_addr));
    OUTCOME_TRY(
        nominal_power_->setMinerPower(miner_addr, updated_nominal_power));

    power::Power consensus_min_power =
        indices_->storagePowerConsensusMinMinerPower();
    if (updated_nominal_power >= consensus_min_power
        && prev_miner_nominal_power < consensus_min_power) {
      num_miners_meeting_min_power_ += 1;
    } else if (updated_nominal_power < consensus_min_power
               && updated_nominal_power >= consensus_min_power) {
      num_miners_meeting_min_power_ -= 1;
    }
    return outcome::success();
  }

  outcome::result<void> StoragePowerActorState::setPowerEntryInternal(
      const primitives::address::Address &miner_addr,
      const power::Power &updated_power) {
    OUTCOME_TRY(prev_miner_power, power_table_->getMinerPower(miner_addr));
    OUTCOME_TRY(power_table_->setMinerPower(miner_addr, updated_power));
    total_network_power_ =
        total_network_power_ + (updated_power - prev_miner_power);
    return outcome::success();
  }

  outcome::result<bool>
  StoragePowerActorState::minerNominalPowerMeetsConsensusMinimum(
      const power::Power &miner_power) {
    // if miner is larger than min power requirement, we're set
    if (miner_power >= kMinMinerSizeStor) {
      return true;
    }

    // otherwise, if another miner meets min power requirement, return false
    if (num_miners_meeting_min_power_ > 0) {
      return false;
    }

    // else if none do, check whether in MIN_MINER_SIZE_TARG miners
    OUTCOME_TRY(size, power_table_->getSize());
    if (size <= kMinMinerSizeTarg) {
      // miner should pass
      return true;
    }

    // get size of MIN_MINER_SIZE_TARGth largest miner
    OUTCOME_TRY(max_power, power_table_->getMaxPower());
    return max_power <= miner_power;
  }

  outcome::result<void> StoragePowerActorState::setClaimedPowerEntryInternal(
      const primitives::address::Address &miner_addr,
      const power::Power &updated_power) {
    return claimed_power_->setMinerPower(miner_addr, updated_power);
  }

  outcome::result<power::Power> StoragePowerActorState::getPowerTotalForMiner(
      const primitives::address::Address &miner_addr) const {
    return power_table_->getMinerPower(miner_addr);
  }

  outcome::result<power::Power> StoragePowerActorState::getNominalPowerForMiner(
      const primitives::address::Address &miner_addr) const {
    return nominal_power_->getMinerPower(miner_addr);
  }

  outcome::result<power::Power> StoragePowerActorState::getClaimedPowerForMiner(
      const primitives::address::Address &miner_addr) const {
    return claimed_power_->getMinerPower(miner_addr);
  }

  outcome::result<void> StoragePowerActorState::addMiner(
      const primitives::address::Address &miner_addr) {
    auto check = power_table_->getMinerPower(miner_addr);
    if (!check.has_error()) {
      return VMExitCode::STORAGE_POWER_ACTOR_ALREADY_EXISTS;
    }

    OUTCOME_TRY(power_table_->setMinerPower(miner_addr, 0));
    OUTCOME_TRY(nominal_power_->setMinerPower(miner_addr, 0));
    OUTCOME_TRY(claimed_power_->setMinerPower(miner_addr, 0));
    return outcome::success();
  }

  outcome::result<void> StoragePowerActorState::removeMiner(
      const primitives::address::Address &miner_addr) {
    OUTCOME_TRY(power_table_->removeMiner(miner_addr));
    OUTCOME_TRY(nominal_power_->removeMiner(miner_addr));
    OUTCOME_TRY(claimed_power_->removeMiner(miner_addr));

    std::string encoded_miner_addr = encodeToByteString(miner_addr);
    OUTCOME_TRY(contains,
                po_st_detected_fault_miners_->contains(encoded_miner_addr));
    if (contains) {
      OUTCOME_TRY(po_st_detected_fault_miners_->remove(encoded_miner_addr));
    }

    return outcome::success();
  }

  outcome::result<void> StoragePowerActorState::addFaultMiner(
      const primitives::address::Address &miner_addr) {
    // check that miner exist
    OUTCOME_TRY(power_table_->getMinerPower(miner_addr));
    return po_st_detected_fault_miners_->set(encodeToByteString(miner_addr),
                                             {});
  }

  outcome::result<std::vector<primitives::address::Address>>
  StoragePowerActorState::getMiners() const {
    return power_table_->getMiners();
  };
}  // namespace fc::vm::actor::builtin::storage_power
