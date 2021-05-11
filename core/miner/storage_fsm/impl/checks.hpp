/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "miner/storage_fsm/types.hpp"

namespace fc::mining::checks {
  using api::FullNodeApi;
  using api::NetworkVersion;
  using api::SectorPreCommitOnChainInfo;
  using common::Buffer;
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::address::Address;
  using primitives::sector::Proof;
  using primitives::tipset::TipsetKey;
  using types::SectorInfo;

  outcome::result<EpochDuration> getMaxProveCommitDuration(
      NetworkVersion network, const std::shared_ptr<SectorInfo> &sector_info);

  outcome::result<void> checkPieces(
      const Address &miner_address,
      const std::shared_ptr<SectorInfo> &sector_info,
      const std::shared_ptr<FullNodeApi> &api);

  outcome::result<boost::optional<SectorPreCommitOnChainInfo>>
  getStateSectorPreCommitInfo(const Address &miner_address,
                              const std::shared_ptr<SectorInfo> &sector_info,
                              const TipsetKey &tipset_key,
                              const std::shared_ptr<FullNodeApi> &api);

  /**
   * Checks that data commitment generated in the sealing process matches
   * pieces, and that the seal ticket isn't expired
   */
  outcome::result<void> checkPrecommit(
      const Address &miner_address,
      const std::shared_ptr<SectorInfo> &sector_info,
      const TipsetKey &tipset_key,
      const ChainEpoch &height,
      const std::shared_ptr<FullNodeApi> &api);

  outcome::result<void> checkCommit(
      const Address &miner_address,
      const std::shared_ptr<SectorInfo> &sector_info,
      const Proof &proof,
      const TipsetKey &tipset_key,
      const std::shared_ptr<FullNodeApi> &api,
      const std::shared_ptr<proofs::ProofEngine> &proofs);

  enum class ChecksError {
    kInvalidDeal = 1,
    kExpiredDeal,
    kInvocationErrored,
    kBadCommD,
    kExpiredTicket,
    kBadTicketEpoch,
    kSectorAllocated,
    kPrecommitOnChain,
    kBadSeed,
    kPrecommitNotFound,
    kBadSealedCid,
    kInvalidProof,
    kCommitWaitFail,
    kMinerVersion,
  };

}  // namespace fc::mining::checks

OUTCOME_HPP_DECLARE_ERROR(fc::mining::checks, ChecksError);
