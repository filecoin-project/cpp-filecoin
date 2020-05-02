/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP

#include "adt/address_key.hpp"
#include "adt/balance_table.hpp"
#include "adt/multimap.hpp"
#include "adt/set.hpp"
#include "adt/uvarint_key.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "crypto/randomness/randomness_provider.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "power/power_table.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/types.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using adt::AddressKeyer;
  using adt::BalanceTable;
  using common::Buffer;
  using power::Power;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::DealWeight;
  using primitives::EpochDuration;
  using primitives::SectorSize;
  using primitives::SectorStorageWeightDesc;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using storage::ipfs::IpfsDatastore;
  using Ipld = storage::ipfs::IpfsDatastore;
  using ChainEpochKeyer = adt::UvarintKeyer;

  // Minimum power of an individual miner to participate in leader election
  // From spec: 100 TiB
  static const power::Power kConsensusMinerMinPower =
      100 * (primitives::BigInt(1) << 40);

  // Minimum number of registered miners for the minimum miner size limit to
  // effectively limit consensus power. From spec: 3
  static const size_t kConsensusMinerMinMiners = 3;

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

  struct StoragePowerActorState {
    inline void load(std::shared_ptr<Ipld> ipld) {
      escrow.load(ipld);
      cron_event_queue.load(ipld);
      po_st_detected_fault_miners.load(ipld);
      claims.load(ipld);
    }

    inline outcome::result<void> flush() {
      OUTCOME_TRY(escrow.flush());
      OUTCOME_TRY(cron_event_queue.flush());
      OUTCOME_TRY(po_st_detected_fault_miners.flush());
      OUTCOME_TRY(claims.flush());
      return outcome::success();
    }

    /**
     * Creates empty StoragePowerActor state
     * @param datastore - ipfs datastore
     * @return an empty initialized state
     */
    static outcome::result<StoragePowerActorState> createEmptyState(
        std::shared_ptr<IpfsDatastore> datastore);

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
    outcome::result<TokenAmount> deleteMiner(const Address &miner_addr);

    /**
     * @brief Checks if miner is present
     * @param miner_addr address
     * @return true if miner is present, false otherwise
     */
    outcome::result<bool> hasMiner(const Address &miner_addr) const;

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
     * Checks if claim with miner address is present
     * @param miner address
     * @return true if claim with address is present or false otherwise
     */
    outcome::result<bool> hasClaim(const Address &miner) const;
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
     * Get all claims
     * @return vector of all claims
     */
    outcome::result<std::vector<Claim>> getClaims() const;

    /**
     * Add event to cron event queue
     * @param epoch
     * @param event
     * @return error in case of failure
     */
    outcome::result<void> appendCronEvent(const ChainEpoch &epoch,
                                          const CronEvent &event);

    /**
     * Get event for epoch
     * @param epoch
     * @return events for epoch
     */
    outcome::result<std::vector<CronEvent>> getCronEvents(
        const ChainEpoch &epoch) const;

    /**
     * Remove event for epoch
     * @param epoch at which events to remove
     * @return error in case of failure
     */
    outcome::result<void> clearCronEvents(const ChainEpoch &epoch);

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
     * @brief Get list of all fault miners
     * @return list of fault miners or error
     */
    outcome::result<std::vector<Address>> getFaultMiners() const;

    /**
     * @brief Get list of all miners in system
     * @return list of miners or error
     */
    outcome::result<std::vector<Address>> getMiners() const;

    /**
     * Compute nominal power: i.e., the power we infer the miner to have (based
     * on the network's PoSt queries), which may not be the same as the claimed
     * power. Currently, the only reason for these to differ is if the miner is
     * in DetectedFault state from a SurprisePoSt challenge
     */
    outcome::result<Power> computeNominalPower(const Address &address) const;

    outcome::result<Power> getTotalNetworkPower() const;

    outcome::result<Claim> assertHasClaim(const Address &address) const;

    outcome::result<void> assertHasEscrow(const Address &address) const;

    /**
     * @brief Decide can a miner participate in consensus
     * @param miner_power is address of the miner
     * @return true or false
     */
    outcome::result<bool> minerNominalPowerMeetsConsensusMinimum(
        const power::Power &miner_power);

    power::Power total_network_power;
    size_t miner_count;
    mutable BalanceTable escrow;
    ChainEpoch last_epoch_tick;
    mutable adt::Map<adt::Array<CronEvent>, ChainEpochKeyer> cron_event_queue;
    mutable adt::Set<AddressKeyer> po_st_detected_fault_miners;
    mutable adt::Map<Claim, AddressKeyer> claims;
    /** Number of miners having proven the minimum consensus power */
    size_t num_miners_meeting_min_power;
  };

  using StoragePowerActor = StoragePowerActorState;

  CBOR_TUPLE(Claim, power, pledge);

  CBOR_TUPLE(CronEvent, miner_address, callback_payload);

  CBOR_TUPLE(StoragePowerActorState,
             total_network_power,
             miner_count,
             escrow,
             cron_event_queue,
             last_epoch_tick,
             po_st_detected_fault_miners,
             claims,
             num_miners_meeting_min_power);

}  // namespace fc::vm::actor::builtin::storage_power

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_STATE_HPP
