/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP

#include "adt/multimap.hpp"
#include "crypto/randomness/randomness_provider.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "power/power_table.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/util.hpp"
#include "vm/indices/indices.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using adt::Multimap;
  using indices::Indices;
  using storage::hamt::Hamt;

  // Min value of power in order to participate in leader election
  // From spec: 100 TiB
  static const power::Power kMinMinerSizeStor =
      100 * (primitives::BigInt(1) << 40);

  // If no one miner has kMinMinerSizeStor then system should choose lower
  // bound as kMinMinerSizeTarg-th miner's power in the top of power table
  // From spec: 3
  static const size_t kMinMinerSizeTarg = 3;

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

  class StoragePowerActorState {
   public:
    StoragePowerActorState(
        std::shared_ptr<Indices> indices,
        std::shared_ptr<crypto::randomness::RandomnessProvider>
            randomness_provider,
        std::shared_ptr<power::PowerTable> escrow_table,
        std::shared_ptr<Multimap> cron_event_queue,
        std::shared_ptr<Hamt> po_st_detected_fault_miners,
        std::shared_ptr<Hamt> claims);

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
    outcome::result<bool> minerNominalPowerMeetsConsensusMinimum(
        const power::Power &miner_power);

    /**
     * @brief Set power value into nominal power table
     * @param miner_addr is miner address
     * @param updated_nominal_power is new power
     * @return success or error
     */
    outcome::result<void> setNominalPowerEntry(
        const primitives::address::Address &miner_addr,
        const power::Power &updated_nominal_power);

    /**
     * @brief Set power value into total power table
     * @param miner_addr is miner address
     * @param updated_power is new power
     * @return success or error
     */
    outcome::result<void> setPowerEntryInternal(
        const primitives::address::Address &miner_addr,
        const power::Power &updated_power);

    /**
     * @brief Set power value into claimed power table
     * @param miner_addr is miner address
     * @param updated_claimed_power is new power
     * @return success or not
     */
    outcome::result<void> setClaimedPowerEntryInternal(
        const primitives::address::Address &miner_addr,
        const power::Power &updated_claimed_power);

    // TODO (a.chernyshov) it's in Runtime - remove
    std::shared_ptr<Indices> indices_;

    // TODO (a.chernyshov) it's in Runtime - remove
    std::shared_ptr<crypto::randomness::RandomnessProvider>
        randomness_provider_;

    template <class Stream, typename>
    friend Stream &operator<<(Stream &&s, const StoragePowerActorState &state);

    template <class Stream, typename>
    friend Stream &operator>>(Stream &&s, StoragePowerActorState &state);

   private:
    power::Power total_network_power_;
    // TODO (a.chernyshov) it is in specs-actor, but can be obtained via HAMT
    size_t miner_count_;

    /**
     * The balances of pledge collateral for each miner actually held by this
     * actor. The sum of the values here should always equal the actor's
     * balance. See Claim for the pledge *requirements* for each actor
     */
    std::shared_ptr<power::PowerTable> escrow_table_;

    /**
     * A queue of events to be triggered by cron, indexed by epoch
     */
    std::shared_ptr<Multimap> cron_event_queue_;

    /**
     * Miners having failed to prove storage
     * As Set, HAMT[Address -> {}]
     */
    std::shared_ptr<Hamt> po_st_detected_fault_miners_;

    /**
     * Claimed power and associated pledge requirements for each miner
     */
    std::shared_ptr<Hamt> claims_;

    /**
     * Number of miners having proven the minimum consensus power
     */
    size_t num_miners_meeting_min_power_;

    // TODO (a.chernyshov) remove
    std::unique_ptr<power::PowerTable> power_table_;
    // TODO (a.chernyshov) remove
    std::unique_ptr<power::PowerTable> claimed_power_;
    // TODO (a.chernyshov) remove
    std::unique_ptr<power::PowerTable> nominal_power_;
  };

  /**
   * CBOR serialization of StoragePowerActorState
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const StoragePowerActorState &state) {
    return s << (s.list() << state.total_network_power_ << state.miner_count_);
  }

  /**
   * CBOR deserialization of ChangeThresholdParameters
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, StoragePowerActorState &state) {
    s.list() >> state.total_network_power_ >> state.miner_count_;
    return s;
  }

}  // namespace fc::vm::actor::builtin::storage_power

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP
