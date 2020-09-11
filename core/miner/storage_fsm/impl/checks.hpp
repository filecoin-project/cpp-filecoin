/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_CHECKS_CHECKS_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_CHECKS_CHECKS_HPP

#include "api/api.hpp"
#include "miner/storage_fsm/types.hpp"

namespace fc::mining::checks {
  using api::Api;
  using common::Buffer;
  using primitives::ChainEpoch;
  using primitives::address::Address;
  using primitives::sector::Proof;
  using primitives::tipset::TipsetKey;
  using types::SectorInfo;

  outcome::result<void> checkPieces(
      const std::shared_ptr<SectorInfo> &sector_info,
      const std::shared_ptr<Api> &api);

  /**
   * Checks that data commitment generated in the sealing process matches
   * pieces, and that the seal ticket isn't expired
   */
  outcome::result<void> checkPrecommit(
      const Address &miner_address,
      const std::shared_ptr<SectorInfo> &sector_info,
      const TipsetKey &tipset_key,
      const ChainEpoch &height,
      const std::shared_ptr<Api> &api);

  outcome::result<void> checkCommit(
      const Address &miner_address,
      const std::shared_ptr<SectorInfo> &sector_info,
      const Proof &proof,
      const TipsetKey &tipset_key,
      const std::shared_ptr<Api> &api);

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
  };

}  // namespace fc::mining::checks

OUTCOME_HPP_DECLARE_ERROR(fc::mining::checks, ChecksError);

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_CHECKS_CHECKS_HPP
