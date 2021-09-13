/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/miner/miner_actor_utils.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using common::Buffer;
  using libp2p::multi::Multiaddress;
  using primitives::ChainEpoch;
  using primitives::address::Address;
  using primitives::sector::RegisteredSealProof;
  using runtime::Runtime;
  using types::miner::CronEventPayload;
  using version::NetworkVersion;

  class MinerUtils : public v0::miner::MinerUtils {
   public:
    explicit MinerUtils(Runtime &r) : v0::miner::MinerUtils(r) {}

    uint64_t getAddressedPartitionsMax() const override;

    ChainEpoch nextProvingPeriodStart(ChainEpoch current_epoch,
                                      ChainEpoch offset) const override;

    ChainEpoch currentProvingPeriodStart(ChainEpoch current_epoch,
                                         ChainEpoch offset) const override;

    outcome::result<uint64_t> currentDeadlineIndex(
        ChainEpoch current_epoch, ChainEpoch period_start) const override;

    outcome::result<void> canPreCommitSealProof(
        RegisteredSealProof seal_proof_type,
        NetworkVersion network_version) const override;

    outcome::result<void> checkPeerInfo(
        const Buffer &peer_id,
        const std::vector<Multiaddress> &multiaddresses) const override;

    outcome::result<void> checkControlAddresses(
        const std::vector<Address> &control_addresses) const override;

   protected:
    outcome::result<Address> getPubkeyAddressFromAccountActor(
        const Address &address) const override;

    outcome::result<void> callPowerEnrollCronEvent(
        ChainEpoch event_epoch, const Buffer &params) const override;
  };
}  // namespace fc::vm::actor::builtin::v2::miner
