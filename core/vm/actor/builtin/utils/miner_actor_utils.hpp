/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/multi/multiaddress.hpp>
#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "primitives/address/address.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/cron_event_payload.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/types/miner/power_pair.hpp"
#include "vm/actor/builtin/types/miner/transit.hpp"
#include "vm/actor/builtin/utils/actor_utils.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/version/version.hpp"

namespace fc::vm::actor::builtin::utils {
  using common::Buffer;
  using common::smoothing::FilterEstimate;
  using libp2p::multi::Multiaddress;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::PoStProof;
  using primitives::sector::RegisteredSealProof;
  using runtime::Runtime;
  using types::miner::CronEventPayload;
  using types::miner::EpochReward;
  using types::miner::PowerPair;
  using types::miner::SectorOnChainInfo;
  using types::miner::TotalPower;
  using version::NetworkVersion;

  class MinerUtils : public ActorUtils {
   public:
    explicit MinerUtils(Runtime &r) : ActorUtils(r) {}

    /**
     * This limits the number of simultaneous fault, recovery, or
     * sector-extension declarations. We set this to same as
     * MaxPartitionsPerDeadline so we can process that many partitions every
     * deadline.
     */
    virtual uint64_t getAddressedPartitionsMax() const = 0;

    inline uint64_t loadPartitionsSectorsMax(
        uint64_t partition_sector_count) const {
      if (partition_sector_count == 0) {
        return 0;
      }
      return std::min(
          types::miner::kAddressedSectorsMax / partition_sector_count,
          getAddressedPartitionsMax());
    }

    /**
     * Resolves an address to an ID address and verifies that it is address of
     * an account or multisig actor
     * @param address to resolve
     * @return resolved address
     */
    virtual outcome::result<Address> resolveControlAddress(
        const Address &address) const = 0;

    /**
     * Resolves an address to an ID address and verifies that it is address of
     * an account actor with an associated BLS key. The worker must be BLS since
     * the worker key will be used alongside a BLS-VRF.
     * @param address to resolve
     * @return resolved address
     */
    virtual outcome::result<Address> resolveWorkerAddress(
        const Address &address) const = 0;

    /**
     * Registers first cron callback for epoch before the first proving period
     * starts.
     */
    virtual outcome::result<void> enrollCronEvent(
        ChainEpoch event_epoch, const CronEventPayload &payload) const = 0;

    virtual outcome::result<void> requestUpdatePower(
        const PowerPair &delta) const = 0;

    /**
     * Assigns proving period offset randomly in the range [0,
     * WPoStProvingPeriod) by hashing the actor's address and current epoch
     * @param current_epoch - current epoch
     * @return random offset
     */
    virtual outcome::result<ChainEpoch> assignProvingPeriodOffset(
        ChainEpoch current_epoch) const = 0;

    /**
     * Computes the epoch at which a proving period should start such that it is
     * greater than the current epoch, and has a defined offset from being an
     * exact multiple of WPoStProvingPeriod. A miner is exempt from Window PoSt
     * until the first full proving period starts.
     */
    virtual ChainEpoch nextProvingPeriodStart(ChainEpoch current_epoch,
                                              ChainEpoch offset) const = 0;

    /**
     * Computes the epoch at which a proving period should start such that it is
     * greater than the current epoch, and has a defined offset from being an
     * exact multiple of WPoStProvingPeriod. A miner is exempt from Window PoSt
     * until the first full proving period starts.
     */
    virtual ChainEpoch currentProvingPeriodStart(ChainEpoch current_epoch,
                                                 ChainEpoch offset) const = 0;

    /**
     * Computes the deadline index for the current epoch for a given period
     * start. currEpoch must be within the proving period that starts at
     * provingPeriodStart to produce a valid index.
     */
    virtual outcome::result<uint64_t> currentDeadlineIndex(
        ChainEpoch current_epoch, ChainEpoch period_start) const = 0;

    virtual outcome::result<void> canPreCommitSealProof(
        RegisteredSealProof seal_proof_type,
        NetworkVersion network_version) const = 0;
    virtual outcome::result<void> checkPeerInfo(
        const Buffer &peer_id,
        const std::vector<Multiaddress> &multiaddresses) const = 0;
    virtual outcome::result<void> checkControlAddresses(
        const std::vector<Address> &control_addresses) const = 0;

    virtual outcome::result<EpochReward> requestCurrentEpochBlockReward()
        const = 0;

    virtual outcome::result<TotalPower> requestCurrentTotalPower() const = 0;

    virtual outcome::result<void> verifyWindowedPost(
        ChainEpoch challenge_epoch,
        const std::vector<SectorOnChainInfo> &sectors,
        const std::vector<PoStProof> &proofs) const = 0;

    virtual outcome::result<void> notifyPledgeChanged(
        const TokenAmount &pledge_delta) const = 0;

   protected:
    virtual outcome::result<Address> getPubkeyAddressFromAccountActor(
        const Address &address) const = 0;
    virtual outcome::result<void> callPowerEnrollCronEvent(
        ChainEpoch event_epoch, const Buffer &params) const = 0;
    virtual outcome::result<void> callPowerUpdateClaimedPower(
        const PowerPair &delta) const = 0;
  };

  using MinerUtilsPtr = std::shared_ptr<MinerUtils>;

}  // namespace fc::vm::actor::builtin::utils
