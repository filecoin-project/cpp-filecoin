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
    static const power::Power kMinMinerSizeStor;
    static const size_t kMinMinerSizeTarg;

    static constexpr VMExitCode OUT_OF_BOUND{1};
    static constexpr VMExitCode ALREADY_EXIST{2};

    StoragePowerActor(std::shared_ptr<Indices> indices,
                      std::shared_ptr<crypto::randomness::RandomnessProvider>
                          randomness_provider);

    /**
     * @brief Select challenge_count miners from power table for the
     * PoSt-Surprise
     * @param challenge_count required number of miner
     * @param randomness for random provider
     * @return set of Miners or OUT_OF_BOUND error if required > actual
     */
    outcome::result<std::vector<primitives::address::Address>>
    selectMinersToSurprise(size_t challenge_count,
                           const crypto::randomness::Randomness &randomness);

    /**
     * @brief Add power to miner by sector power value
     * @param miner_addr is address of miner
     * @param storage_weight_desc is description of sector
     * @return success or error
     */
    outcome::result<void> addClaimedPowerForSector(
        const primitives::address::Address &miner_addr,
        const SectorStorageWeightDesc &storage_weight_desc);

    /**
     * @brief Deduct miner power by sector power value
     * @param miner_addr is address of miner
     * @param storage_weight_desc is description of sector
     * @return success or error
     */
    outcome::result<void> deductClaimedPowerForSectorAssert(
        const primitives::address::Address &miner_addr,
        const SectorStorageWeightDesc &storage_weight_desc);

    /**
     * @brief Get total power of miner
     * @param miner_addr is address of miner
     * @return power or error
     */
    outcome::result<power::Power> getPowerTotalForMiner(
        const primitives::address::Address &miner_addr) const;

    /**
     * @brief Get nominal power of miner
     * @param miner_addr is address of miner
     * @return power or error
     */
    outcome::result<power::Power> getNominalPowerForMiner(
        const primitives::address::Address &miner_addr) const;

    /**
     * @brief Get claimed power of miner
     * @param miner_addr is address of miner
     * @return power or error
     */
    outcome::result<power::Power> getClaimedPowerForMiner(
        const primitives::address::Address &miner_addr) const;

    /**
     * @brief Add miner to system
     * @param miner_addr is address of miner
     * @return power or error ALREADY_EXIST
     */
    outcome::result<void> addMiner(
        const primitives::address::Address &miner_addr);

    /**
     * @brief Remove miner from system
     * @param miner_addr is address of miner
     * @return success or error NO_SUCH_MINER
     */
    outcome::result<void> removeMiner(
        const primitives::address::Address &miner_addr);

    /**
     * @brief Add miner to miners list failed proof
     * @param miner_addr is address of miner
     * @return success or error NO_SUCH_MINER
     */
    outcome::result<void> addFaultMiner(
        const primitives::address::Address &miner_addr);

    /**
     * @brief Get list of all miners in system
     * @return list of miners or error
     */
    outcome::result<std::vector<primitives::address::Address>> getMiners()
        const;

   private:
    /**
     * @brief Tables synchronization for a miner
     * @param miner_addr is address of the miner
     * @return success or error
     */
    outcome::result<void> updatePowerEntriesFromClaimedPower(
        const primitives::address::Address &miner_addr);

    /**
     * @brief Decide can a miner participate in consensus
     * @param miner_power is address of the miner
     * @return true or false
     */
    bool minerNominalPowerMeetsConsensusMinimum(power::Power miner_power);

    /**
     * @brief Set power value into nominal power table
     * @param miner_addr is miner address
     * @param updated_nominal_power is new power
     * @return success or error
     */
    outcome::result<void> setNominalPowerEntry(
        const primitives::address::Address &miner_addr,
        power::Power updated_nominal_power);

    /**
     * @brief Set power value into total power table
     * @param miner_addr is miner address
     * @param updated_power is new power
     * @return success or error
     */
    outcome::result<void> setPowerEntryInternal(
        const primitives::address::Address &miner_addr,
        power::Power updated_power);

    /**
     * @brief Set power value into claimed power table
     * @param miner_addr is miner address
     * @param updated_claimed_power is new power
     * @return success or not
     */
    outcome::result<void> setClaimedPowerEntryInternal(
        const primitives::address::Address &miner_addr,
        power::Power updated_claimed_power);

    std::shared_ptr<Indices> indices_;

    std::shared_ptr<crypto::randomness::RandomnessProvider>
        randomness_provider_;

    power::Power total_network_power_;
    std::unique_ptr<power::PowerTable> power_table_;
    std::unique_ptr<power::PowerTable> claimed_power_;
    std::unique_ptr<power::PowerTable> nominal_power_;

    std::set<primitives::address::Address> po_st_detected_fault_miners_;

    int num_miners_meeting_min_power;
  };
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_HPP
