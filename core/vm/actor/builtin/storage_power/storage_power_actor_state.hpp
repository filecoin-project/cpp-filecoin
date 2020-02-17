/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP

#include "adt/balance_table_hamt.hpp"
#include "adt/multimap.hpp"
#include "crypto/randomness/randomness_provider.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "power/power_table.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/indices/indices.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using adt::BalanceTableHamt;
  using adt::Multimap;
  using common::Buffer;
  using indices::Indices;
  using power::Power;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::address::Address;
  using storage::hamt::Hamt;
  using storage::ipfs::IpfsDatastore;
  using TokenAmount = primitives::BigInt;

  // Minimum power of an individual miner to participate in leader election
  // From spec: 100 TiB
  static const power::Power kConsensusMinerMinPower =
      100 * (primitives::BigInt(1) << 40);

  // Minimum number of registered miners for the minimum miner size limit to
  // effectively limit consensus power. From spec: 3
  static const size_t kConsensusMinerMinMiners = 3;

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

  struct Claim {
    // Sum of power for a miner's sectors
    Power power;
    // Sum of pledge requirement for a miner's sectors
    TokenAmount pledge;

    inline bool operator==(const Claim &other) const {
      return power == other.power && pledge == other.pledge;
    }
  };

  struct CronEvent {
    Address miner_address;
    Buffer callback_payload;
  };

  class StoragePowerActorState {
   public:
    StoragePowerActorState(
        std::shared_ptr<Indices> indices,
        std::shared_ptr<crypto::randomness::RandomnessProvider>
            randomness_provider,
        std::shared_ptr<IpfsDatastore> datastore,
        const CID &escrow_table_cid,
        const CID &cron_event_queue_cid,
        const CID &po_st_detected_fault_miners_cid,
        const CID &claims_cid);

    /**
     * @brief Add miner to system
     * @param miner_addr is address of miner
     * @return power or error ALREADY_EXIST
     */
    outcome::result<void> addMiner(const Address &miner_addr);

    /**
     * @brief Remove miner from system
     * @param miner_addr is address of miner
     * @return success or error NO_SUCH_MINER
     */
    outcome::result<void> deleteMiner(const Address &miner_addr);

    /**
     * Return miner balance
     * @param miner address
     * @return balance
     */
    outcome::result<TokenAmount> getMinerBalance(const Address &miner) const;

    /**
     * Set miner balance
     */
    outcome::result<void> setMinerBalance(const Address &miner,
                                          const TokenAmount &balance);

    /**
     * Add to miner balance
     */
    outcome::result<void> addMinerBalance(const Address &miner,
                                          const TokenAmount &amount);

    /**
     * Subtracts up to the specified amount from a balance, without reducing the
     * balance below some minimum
     * @returns the amount subtracted (always positive or zero)
     */
    outcome::result<TokenAmount> subtractMinerBalance(
        const Address &miner,
        const TokenAmount &amount,
        const TokenAmount &balance_floor);

    /**
     * Add new miner claim or override old one
     * @param miner address
     * @param claim
     * @return error in case of failure
     */
    outcome::result<void> setClaim(const Address &miner, const Claim &claim);

    /**
     * Get claim for a miner
     * @param miner address
     * @return claim
     */
    outcome::result<Claim> getClaim(const Address &miner);

    /**
     * Deletes claim
     * @param miner address
     * @return error in case of failure
     */
    outcome::result<void> deleteClaim(const Address &miner);

    /**
     * Add miner claim
     * @param miner - address
     * @param power - to add to claim power
     * @param pledge - to add to claim pledge
     * @return error in case of failure
     */
    outcome::result<void> addToClaim(const Address &miner,
                                     const Power &power,
                                     const TokenAmount &pledge);

    /**
     * Add event to cron event queue
     * @param epoch
     * @param event
     * @return error in case of failure
     */
    outcome::result<void> appendCronEvent(const ChainEpoch &epoch,
                                          const CronEvent &event);

    /**
     * @brief Add miner to miners list failed proof
     * @param miner_addr is address of miner
     * @return success or error NO_SUCH_MINER
     */
    outcome::result<void> addFaultMiner(const Address &miner_addr);

    /**
     * Checks if miner is fault
     * @param miner_addr miner address
     * @return true if miner is fault
     */
    outcome::result<bool> hasFaultMiner(const Address &miner_addr) const;

    /**
     * @brief Remove miner from miners list failed proof
     * @param miner_addr is address of miner
     * @return success or error NO_SUCH_MINER
     */
    outcome::result<void> deleteFaultMiner(const Address &miner_addr);

    /**
     * @brief Get list of all miners in system
     * @return list of miners or error
     */
    outcome::result<std::vector<primitives::address::Address>> getMiners()
        const;

    /**
     * Compute nominal power: i.e., the power we infer the miner to have (based
     * on the network's PoSt queries), which may not be the same as the claimed
     * power. Currently, the only reason for these to differ is if the miner is
     * in DetectedFault state from a SurprisePoSt challenge
     */
    outcome::result<Power> computeNominalPower(const Address &address) const;

    template <class Stream, typename>
    friend Stream &operator<<(Stream &&s, const StoragePowerActorState &state);

    template <class Stream, typename>
    friend Stream &operator>>(Stream &&s, StoragePowerActorState &state);

   private:
    /**
     * @brief Decide can a miner participate in consensus
     * @param miner_power is address of the miner
     * @return true or false
     */
    outcome::result<bool> minerNominalPowerMeetsConsensusMinimum(
        const power::Power &miner_power);

    /**
     * Datastore for internal state
     */
    std::shared_ptr<IpfsDatastore> datastore_;

    // TODO (a.chernyshov) it's in Runtime - remove
    std::shared_ptr<Indices> indices_;

    // TODO (a.chernyshov) it's in Runtime - remove
    std::shared_ptr<crypto::randomness::RandomnessProvider>
        randomness_provider_;

    power::Power total_network_power_;
    // TODO (a.chernyshov) it is in specs-actor, but can be obtained via HAMT
    size_t miner_count_;

    /**
     * The balances of pledge collateral for each miner actually held by this
     * actor. The sum of the values here should always equal the actor's
     * balance. See Claim for the pledge *requirements* for each actor
     */
    std::shared_ptr<BalanceTableHamt> escrow_table_;

    /**
     * A queue of events to be triggered by cron, indexed by epoch
     */
    std::shared_ptr<Multimap> cron_event_queue_;
    CID cron_event_queue_cid_;

    /**
     * Miners having failed to prove storage
     * As Set, HAMT[Address -> {}]
     */
    std::shared_ptr<Hamt> po_st_detected_fault_miners_;
    CID po_st_detected_fault_miners_cid_;

    /**
     * Claimed power and associated pledge requirements for each miner
     */
    std::shared_ptr<Hamt> claims_;
    CID claims_cid_;

    /**
     * Number of miners having proven the minimum consensus power
     */
    size_t num_miners_meeting_min_power_;
  };

  /**
   * CBOR serialization of Claim
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Claim &claim) {
    return s << (s.list() << claim.power << claim.pledge);
  }

  /**
   * CBOR deserialization of Claim
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Claim &claim) {
    s.list() >> claim.power >> claim.pledge;
    return s;
  }

  /**
   * CBOR serialization of CronEvent
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const CronEvent &event) {
    return s << (s.list() << event.miner_address << event.callback_payload);
  }

  /**
   * CBOR deserialization of CronEvent
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, CronEvent &event) {
    s.list() >> event.miner_address >> event.callback_payload;
    return s;
  }

  /**
   * CBOR serialization of StoragePowerActorState
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const StoragePowerActorState &state) {
    return s << (s.list() << state.total_network_power_ << state.miner_count_
                          << *state.escrow_table_ << state.cron_event_queue_cid_
                          << state.po_st_detected_fault_miners_cid_
                          << state.claims_cid_
                          << state.num_miners_meeting_min_power_);
  }

  /**
   * CBOR deserialization of ChangeThresholdParameters
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, StoragePowerActorState &state) {
    s.list() >> state.total_network_power_ >> state.miner_count_
        >> *state.escrow_table_ >> state.cron_event_queue_cid_
        >> state.po_st_detected_fault_miners_cid_ >> state.claims_cid_
        >> state.num_miners_meeting_min_power_;

    state.cron_event_queue_ = std::make_shared<Multimap>(
        state.datastore_, state.cron_event_queue_cid_);
    state.po_st_detected_fault_miners_ = std::make_shared<Hamt>(
        state.datastore_, state.po_st_detected_fault_miners_cid_);
    state.claims_ = std::make_shared<Hamt>(state.datastore_, state.claims_cid_);

    return s;
  }

}  // namespace fc::vm::actor::builtin::storage_power

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP
