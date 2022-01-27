/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/utils/miner_actor_utils.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using libp2p::multi::Multiaddress;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::PoStProof;
  using primitives::sector::RegisteredSealProof;
  using runtime::Runtime;
  using states::MinerActorStatePtr;
  using types::DealWeights;
  using types::EpochReward;
  using types::TotalPower;
  using types::Universal;
  using types::miner::CronEventPayload;
  using types::miner::PowerPair;
  using types::miner::SectorOnChainInfo;
  using types::miner::SectorPreCommitInfo;
  using version::NetworkVersion;

  class MinerUtils : public utils::MinerUtils {
   public:
    explicit MinerUtils(Runtime &r) : utils::MinerUtils(r) {}

    uint64_t getAddressedPartitionsMax() const override;

    outcome::result<Address> resolveControlAddress(
        const Address &address) const override;

    outcome::result<Address> resolveWorkerAddress(
        const Address &address) const override;

    outcome::result<void> enrollCronEvent(
        ChainEpoch event_epoch, const CronEventPayload &payload) const override;

    outcome::result<void> requestUpdatePower(
        const PowerPair &delta) const override;

    outcome::result<ChainEpoch> assignProvingPeriodOffset(
        ChainEpoch current_epoch) const override;

    ChainEpoch nextProvingPeriodStart(ChainEpoch current_epoch,
                                      ChainEpoch offset) const override;

    ChainEpoch currentProvingPeriodStart(ChainEpoch current_epoch,
                                         ChainEpoch offset) const override;

    outcome::result<void> validateExpiration(
        ChainEpoch activation,
        ChainEpoch expiration,
        RegisteredSealProof seal_proof) const override;

    outcome::result<Universal<SectorOnChainInfo>> validateReplaceSector(
        MinerActorStatePtr &state,
        const SectorPreCommitInfo &params) const override;

    outcome::result<uint64_t> currentDeadlineIndex(
        ChainEpoch current_epoch, ChainEpoch period_start) const override;

    outcome::result<void> canPreCommitSealProof(
        RegisteredSealProof seal_proof_type,
        NetworkVersion network_version) const override;

    outcome::result<void> checkPeerInfo(
        const Bytes &peer_id,
        const std::vector<Multiaddress> &multiaddresses) const override;

    outcome::result<void> checkControlAddresses(
        const std::vector<Address> &control_addresses) const override;

    outcome::result<EpochReward> requestCurrentEpochBlockReward()
        const override;

    outcome::result<TotalPower> requestCurrentTotalPower() const override;

    outcome::result<DealWeights> requestDealWeight(
        const std::vector<DealId> &deals,
        ChainEpoch sector_start,
        ChainEpoch sector_expiry) const override;

    outcome::result<void> verifyWindowedPost(
        ChainEpoch challenge_epoch,
        const std::vector<Universal<SectorOnChainInfo>> &sectors,
        const std::vector<PoStProof> &proofs) const override;

    outcome::result<void> notifyPledgeChanged(
        const TokenAmount &pledge_delta) const override;

   protected:
    outcome::result<Address> getPubkeyAddressFromAccountActor(
        const Address &address) const override;

    outcome::result<void> callPowerEnrollCronEvent(
        ChainEpoch event_epoch, const Bytes &params) const override;

    outcome::result<void> callPowerUpdateClaimedPower(
        const PowerPair &delta) const override;
  };
}  // namespace fc::vm::actor::builtin::v0::miner
