/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/storage_power_actor.hpp"
#include <power/impl/power_table_impl.hpp>
#include "vm/indices/indices.hpp"

fc::outcome::result<std::vector<fc::primitives::address::Address>>
fc::vm::actor::StoragePowerActor::selectMinersToSurprise(
    size_t challenge_count,
    const fc::crypto::randomness::Randomness &randomness) {
  std::vector<fc::primitives::address::Address> selected_miners;

  if (power_table_->getSize() < challenge_count) return OUT_OF_BOUND;

  OUTCOME_TRY(all_miners, power_table_->getMiners());

  if (challenge_count == all_miners.size()) return std::move(all_miners);

  for (size_t chall = 0; chall < challenge_count; chall++) {
    int miner_index =
        randomness_provider_->randomInt(randomness, chall, all_miners.size());
    auto potential_challengee = all_miners[miner_index];

    all_miners.erase(all_miners.begin() + miner_index);

    selected_miners.push_back(potential_challengee);
  }

  return selected_miners;
}

fc::outcome::result<void>
fc::vm::actor::StoragePowerActor::addClaimedPowerForSector(
    const fc::primitives::address::Address &miner_addr,
    const SectorStorageWeightDesc &storage_weight_desc) {
  fc::primitives::BigInt sector_power =
      indices_->consensusPowerForStorageWeight(storage_weight_desc);

  OUTCOME_TRY(miner_power, claimed_power_->getMinerPower(miner_addr));

  OUTCOME_TRY(
      setClaimedPowerEntryInternal(miner_addr, miner_power + sector_power));

  return updatePowerEntriesFromClaimedPower(miner_addr);
}

fc::outcome::result<void>
fc::vm::actor::StoragePowerActor::deductClaimedPowerForSectorAssert(
    const fc::primitives::address::Address &miner_addr,
    const SectorStorageWeightDesc &storage_weight_desc) {
  fc::primitives::BigInt sector_power =
      indices_->consensusPowerForStorageWeight(storage_weight_desc);

  OUTCOME_TRY(miner_power, claimed_power_->getMinerPower(miner_addr));

  OUTCOME_TRY(
      setClaimedPowerEntryInternal(miner_addr, miner_power - sector_power));

  return updatePowerEntriesFromClaimedPower(miner_addr);
}

fc::outcome::result<void>
fc::vm::actor::StoragePowerActor::updatePowerEntriesFromClaimedPower(
    const fc::primitives::address::Address &miner_addr) {
  OUTCOME_TRY(claimed_power, claimed_power_->getMinerPower(miner_addr));

  fc::primitives::BigInt nominal_power = claimed_power;
  if (std::find(po_st_detected_fault_miners_.cbegin(),
                po_st_detected_fault_miners_.cend(),
                miner_addr)
      != po_st_detected_fault_miners_.cend()) {
    nominal_power = 0;
  }
  OUTCOME_TRY(setNominalPowerEntry(miner_addr, nominal_power));

  // Compute actual (consensus) power, i.e., votes in leader election.
  fc::primitives::BigInt power = nominal_power;
  if (!minerNominalPowerMeetsConsensusMinimum(nominal_power)) {
    power = 0;
  }

  // TODO: FROM DOCS Decide effect of undercollateralization on (consensus)
  // power.

  return setPowerEntryInternal(miner_addr, power);
}

fc::outcome::result<void>
fc::vm::actor::StoragePowerActor::setNominalPowerEntry(
    const fc::primitives::address::Address &miner_addr,
    fc::primitives::BigInt updated_nominal_power) {
  OUTCOME_TRY(prev_miner_nominal_power,
              nominal_power_->getMinerPower(miner_addr));
  OUTCOME_TRY(nominal_power_->setMinerPower(miner_addr, updated_nominal_power));

  fc::primitives::BigInt consensus_min_power =
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

fc::outcome::result<void>
fc::vm::actor::StoragePowerActor::setPowerEntryInternal(
    const fc::primitives::address::Address &miner_addr,
    fc::primitives::BigInt updated_power) {
  OUTCOME_TRY(prev_miner_power, power_table_->getMinerPower(miner_addr));
  OUTCOME_TRY(power_table_->setMinerPower(miner_addr, updated_power));
  total_network_power_ =
      total_network_power_ + (updated_power - prev_miner_power);
  return outcome::success();
}

bool fc::vm::actor::StoragePowerActor::minerNominalPowerMeetsConsensusMinimum(
    fc::primitives::BigInt miner_power) {
  // TODO: move to constants
  fc::primitives::BigInt MIN_MINER_SIZE_STOR =
      100 * (fc::primitives::BigInt(1) << 40);
  size_t MIN_MINER_SIZE_TARG = 3;

  // if miner is larger than min power requirement, we're set
  if (miner_power >= MIN_MINER_SIZE_STOR) {
    return true;
  }

  // otherwise, if another miner meets min power requirement, return false
  if (num_miners_meeting_min_power > 0) {
    return false;
  }

  // else if none do, check whether in MIN_MINER_SIZE_TARG miners
  if (power_table_->getSize() <= MIN_MINER_SIZE_TARG) {
    // miner should pass
    return true;
  }

  // get size of MIN_MINER_SIZE_TARGth largest miner
  return miner_power >= power_table_->getMaxPower();
}

fc::outcome::result<void>
fc::vm::actor::StoragePowerActor::setClaimedPowerEntryInternal(
    const fc::primitives::address::Address &miner_addr,
    fc::primitives::BigInt updated__power) {
  return claimed_power_->setMinerPower(miner_addr, updated__power);
}
fc::outcome::result<fc::primitives::BigInt>
fc::vm::actor::StoragePowerActor::getPowerTotalForMiner(
    const fc::primitives::address::Address &miner_addr) const {
  return power_table_->getMinerPower(miner_addr);
}

fc::outcome::result<fc::primitives::BigInt>
fc::vm::actor::StoragePowerActor::getNominalPowerForMiner(
    const fc::primitives::address::Address &miner_addr) const {
  return nominal_power_->getMinerPower(miner_addr);
}

fc::outcome::result<fc::primitives::BigInt>
fc::vm::actor::StoragePowerActor::getClaimedPowerForMiner(
    const fc::primitives::address::Address &miner_addr) const {
  return claimed_power_->getMinerPower(miner_addr);
}

fc::outcome::result<void> fc::vm::actor::StoragePowerActor::addMiner(
    const fc::primitives::address::Address &miner_addr) {
  OUTCOME_TRY(power_table_->setMinerPower(miner_addr, 0));
  OUTCOME_TRY(nominal_power_->setMinerPower(miner_addr, 0));
  OUTCOME_TRY(claimed_power_->setMinerPower(miner_addr, 0));
  return outcome::success();
}

fc::outcome::result<void> fc::vm::actor::StoragePowerActor::removeMiner(
    const fc::primitives::address::Address &miner_addr) {
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

fc::vm::actor::StoragePowerActor::StoragePowerActor(
    std::shared_ptr<fc::vm::Indices> indices,
    std::shared_ptr<fc::crypto::randomness::RandomnessProvider>
        randomness_provider)
    : indices_(indices), randomness_provider_(randomness_provider) {
  total_network_power_ = 0;

  power_table_ = std::make_unique<fc::power::PowerTableImpl>();
  claimed_power_ = std::make_unique<fc::power::PowerTableImpl>();
  nominal_power_ = std::make_unique<fc::power::PowerTableImpl>();

  po_st_detected_fault_miners_ = {};

  num_miners_meeting_min_power = 0;
}
fc::outcome::result<void> fc::vm::actor::StoragePowerActor::addFaultMiner(
    const fc::primitives::address::Address &miner_addr) {
  // check that miner exist
  OUTCOME_TRY(power_table_->getMinerPower(miner_addr));

  po_st_detected_fault_miners_.insert(miner_addr);

  return outcome::success();
}
fc::outcome::result<std::vector<fc::primitives::address::Address>>
fc::vm::actor::StoragePowerActor::getMiners() const {
  return power_table_->getMiners();
};
