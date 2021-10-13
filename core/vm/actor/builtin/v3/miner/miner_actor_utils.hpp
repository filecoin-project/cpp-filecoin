/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/miner/miner_actor_utils.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using common::Buffer;
  using libp2p::multi::Multiaddress;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::address::Address;
  using primitives::sector::RegisteredSealProof;
  using runtime::Runtime;
  using states::MinerActorStatePtr;
  using types::DealWeights;
  using types::EpochReward;
  using types::TotalPower;
  using types::miner::CronEventPayload;
  using types::miner::PowerPair;
  using types::miner::SectorOnChainInfo;
  using types::miner::SectorPreCommitInfo;
  using version::NetworkVersion;

  class MinerUtils : public v2::miner::MinerUtils {
   public:
    explicit MinerUtils(Runtime &r) : v2::miner::MinerUtils(r) {}

    outcome::result<SectorOnChainInfo> validateReplaceSector(
        MinerActorStatePtr &state,
        const SectorPreCommitInfo &params) const override;

    outcome::result<void> canPreCommitSealProof(
        RegisteredSealProof seal_proof_type,
        NetworkVersion network_version) const override;

    outcome::result<EpochReward> requestCurrentEpochBlockReward()
        const override;

    outcome::result<TotalPower> requestCurrentTotalPower() const override;

    outcome::result<void> notifyPledgeChanged(
        const TokenAmount &pledge_delta) const override;

    outcome::result<DealWeights> requestDealWeight(
        const std::vector<DealId> &deals,
        ChainEpoch sector_start,
        ChainEpoch sector_expiry) const override;

   protected:
    outcome::result<Address> getPubkeyAddressFromAccountActor(
        const Address &address) const override;

    outcome::result<void> callPowerEnrollCronEvent(
        ChainEpoch event_epoch, const Buffer &params) const override;

    outcome::result<void> callPowerUpdateClaimedPower(
        const PowerPair &delta) const override;
  };
}  // namespace fc::vm::actor::builtin::v3::miner
