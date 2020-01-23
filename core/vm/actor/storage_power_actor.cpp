/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/storage_power_actor.hpp"

#include "power/impl/power_table_impl.hpp"
#include "vm/indices/indices.hpp"

namespace fc::vm::actor {

  const power::Power StoragePowerActor::kMinMinerSizeStor =
      100 * (primitives::BigInt(1) << 40);

  const size_t StoragePowerActor::kMinMinerSizeTarg = 3;

  outcome::result<std::vector<primitives::address::Address>>
  StoragePowerActor::selectMinersToSurprise(
      size_t challenge_count,
      const crypto::randomness::Randomness &randomness) {
    std::vector<primitives::address::Address> selected_miners;

    if (power_table_->getSize() < challenge_count) return OUT_OF_BOUND;

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

  outcome::result<void> StoragePowerActor::addClaimedPowerForSector(
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

  outcome::result<void> StoragePowerActor::deductClaimedPowerForSectorAssert(
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

  outcome::result<void> StoragePowerActor::updatePowerEntriesFromClaimedPower(
      const primitives::address::Address &miner_addr) {
    OUTCOME_TRY(claimed_power, claimed_power_->getMinerPower(miner_addr));

    power::Power nominal_power = claimed_power;
    if (std::find(po_st_detected_fault_miners_.cbegin(),
                  po_st_detected_fault_miners_.cend(),
                  miner_addr)
        != po_st_detected_fault_miners_.cend()) {
      nominal_power = 0;
    }
    OUTCOME_TRY(setNominalPowerEntry(miner_addr, nominal_power));

    // Compute actual (consensus) power, i.e., votes in leader election.
    power::Power power = nominal_power;
    if (!minerNominalPowerMeetsConsensusMinimum(nominal_power)) {
      power = 0;
    }

    // TODO(artyom-yurin): [FIL-136] FROM DOCS Decide effect of
    // undercollateralization on (consensus) power.

    return setPowerEntryInternal(miner_addr, power);
  }

  outcome::result<void> StoragePowerActor::setNominalPowerEntry(
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
      num_miners_meeting_min_power += 1;
    } else if (updated_nominal_power < consensus_min_power
               && updated_nominal_power >= consensus_min_power) {
      num_miners_meeting_min_power -= 1;
    }
    return outcome::success();
  }

  outcome::result<void> StoragePowerActor::setPowerEntryInternal(
      const primitives::address::Address &miner_addr,
      const power::Power &updated_power) {
    OUTCOME_TRY(prev_miner_power, power_table_->getMinerPower(miner_addr));
    OUTCOME_TRY(power_table_->setMinerPower(miner_addr, updated_power));
    total_network_power_ =
        total_network_power_ + (updated_power - prev_miner_power);
    return outcome::success();
  }

  bool StoragePowerActor::minerNominalPowerMeetsConsensusMinimum(
      const power::Power &miner_power) {
    // if miner is larger than min power requirement, we're set
    if (miner_power >= kMinMinerSizeStor) {
      return true;
    }

    // otherwise, if another miner meets min power requirement, return false
    if (num_miners_meeting_min_power > 0) {
      return false;
    }

    // else if none do, check whether in MIN_MINER_SIZE_TARG miners
    if (power_table_->getSize() <= kMinMinerSizeTarg) {
      // miner should pass
      return true;
    }

    // get size of MIN_MINER_SIZE_TARGth largest miner
    return miner_power >= power_table_->getMaxPower();
  }

  outcome::result<void> StoragePowerActor::setClaimedPowerEntryInternal(
      const primitives::address::Address &miner_addr,
      const power::Power &updated_power) {
    return claimed_power_->setMinerPower(miner_addr, updated_power);
  }
  outcome::result<power::Power> StoragePowerActor::getPowerTotalForMiner(
      const primitives::address::Address &miner_addr) const {
    return power_table_->getMinerPower(miner_addr);
  }

  outcome::result<power::Power> StoragePowerActor::getNominalPowerForMiner(
      const primitives::address::Address &miner_addr) const {
    return nominal_power_->getMinerPower(miner_addr);
  }

  outcome::result<power::Power> StoragePowerActor::getClaimedPowerForMiner(
      const primitives::address::Address &miner_addr) const {
    return claimed_power_->getMinerPower(miner_addr);
  }

  outcome::result<void> StoragePowerActor::addMiner(
      const primitives::address::Address &miner_addr) {
    auto check = power_table_->getMinerPower(miner_addr);
    if (!check.has_error()) return ALREADY_EXISTS;

    OUTCOME_TRY(power_table_->setMinerPower(miner_addr, 0));
    OUTCOME_TRY(nominal_power_->setMinerPower(miner_addr, 0));
    OUTCOME_TRY(claimed_power_->setMinerPower(miner_addr, 0));
    return outcome::success();
  }

  outcome::result<void> StoragePowerActor::removeMiner(
      const primitives::address::Address &miner_addr) {
    OUTCOME_TRY(power_table_->removeMiner(miner_addr));
    OUTCOME_TRY(nominal_power_->removeMiner(miner_addr));
    OUTCOME_TRY(claimed_power_->removeMiner(miner_addr));
    auto position = std::find(po_st_detected_fault_miners_.cbegin(),
                              po_st_detected_fault_miners_.cend(),
                              miner_addr);
    if (position != po_st_detected_fault_miners_.cend())
      po_st_detected_fault_miners_.erase(position);
    return outcome::success();
  }

  StoragePowerActor::StoragePowerActor(
      std::shared_ptr<Indices> indices,
      std::shared_ptr<crypto::randomness::RandomnessProvider>
          randomness_provider)
      : indices_(std::move(indices)),
        randomness_provider_(std::move(randomness_provider)),
        total_network_power_(0),
        power_table_(std::make_unique<power::PowerTableImpl>()),
        claimed_power_(std::make_unique<power::PowerTableImpl>()),
        nominal_power_(std::make_unique<power::PowerTableImpl>()),
        po_st_detected_fault_miners_({}),
        num_miners_meeting_min_power(0) {}

  outcome::result<void> StoragePowerActor::addFaultMiner(
      const primitives::address::Address &miner_addr) {
    // check that miner exist
    OUTCOME_TRY(power_table_->getMinerPower(miner_addr));

    po_st_detected_fault_miners_.insert(miner_addr);

    return outcome::success();
  }

  outcome::result<std::vector<primitives::address::Address>>
  StoragePowerActor::getMiners() const {
    return power_table_->getMiners();
  };
}  // namespace fc::vm::actor
