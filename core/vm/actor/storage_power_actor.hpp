/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_HPP

#include "crypto/randomness/randomness_provider.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "power/power_table.hpp"
#include "vm/actor/util.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/indices/indices.hpp"

namespace fc::vm::actor {

  enum SpaMethods {
    CONSTRUCTOR = 1,
    CREATE_STORAGE_MINER,
    ARBITRATE_CONSENSUS_FAULT,
    UPDATE_STORAGE,
    GET_TOTAL_STORAGE,
    POWER_LOOKUP,
    IS_VALID_MINER,
    PLEDGE_COLLATERAL_FOR_SIZE,
    CHECK_PROOF_SUBMISSIONS
  };

  class StoragePowerActor {
   public:
    static constexpr VMExitCode OUT_OF_BOUND{1};

    StoragePowerActor(
        std::shared_ptr<fc::vm::Indices> indices,
        std::shared_ptr<fc::crypto::randomness::RandomnessProvider>
            randomness_provider);

    outcome::result<std::vector<primitives::address::Address>>
    selectMinersToSurprise(
        size_t challenge_count,
        const fc::crypto::randomness::Randomness &randomness);

    outcome::result<void> addClaimedPowerForSector(
        const primitives::address::Address &miner_addr,
        const SectorStorageWeightDesc &storage_weight_desc);

    outcome::result<void> deductClaimedPowerForSectorAssert(
        const primitives::address::Address &miner_addr,
        const SectorStorageWeightDesc &storage_weight_desc);

    outcome::result<fc::primitives::BigInt> getPowerTotalForMiner(
        const primitives::address::Address &miner_addr) const;

    outcome::result<fc::primitives::BigInt> getNominalPowerForMiner(
        const primitives::address::Address &miner_addr) const;

    outcome::result<fc::primitives::BigInt> getClaimedPowerForMiner(
        const primitives::address::Address &miner_addr) const;

    outcome::result<void> addMiner(
        const primitives::address::Address &miner_addr);

    outcome::result<void> removeMiner(
        const primitives::address::Address &miner_addr);

    outcome::result<void> addFaultMiner(
        const primitives::address::Address &miner_addr);

    outcome::result<std::vector<primitives::address::Address>> getMiners()
        const;

   private:
    outcome::result<void> updatePowerEntriesFromClaimedPower(
        const primitives::address::Address &miner_addr);

    bool minerNominalPowerMeetsConsensusMinimum(
        fc::primitives::BigInt miner_power);

    outcome::result<void> setNominalPowerEntry(
        const primitives::address::Address &miner_addr,
        fc::primitives::BigInt updated_nominal_power);

    outcome::result<void> setPowerEntryInternal(
        const primitives::address::Address &miner_addr,
        fc::primitives::BigInt updated_power);

    outcome::result<void> setClaimedPowerEntryInternal(
        const primitives::address::Address &miner_addr,
        fc::primitives::BigInt updated_claimed_power);

    fc::primitives::BigInt total_network_power_;
    std::unique_ptr<fc::power::PowerTable> power_table_;
    std::unique_ptr<fc::power::PowerTable> claimed_power_;
    std::unique_ptr<fc::power::PowerTable> nominal_power_;

    std::set<primitives::address::Address> po_st_detected_fault_miners_;

    int num_miners_meeting_min_power;

    // TODO: remove after indices will be implemented
    std::shared_ptr<fc::vm::Indices> indices_;

    std::shared_ptr<fc::crypto::randomness::RandomnessProvider>
        randomness_provider_;
  };
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_HPP
