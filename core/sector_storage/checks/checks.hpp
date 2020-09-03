/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_CHECKS_CHECKS_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_CHECKS_CHECKS_HPP

#include "api/api.hpp"
#include "miner/storage_fsm/impl/sealing_impl.hpp"

namespace fc::sector_storage::checks {
  using api::Api;
  using common::Buffer;
  using mining::SectorInfo;
  using primitives::ChainEpoch;
  using primitives::address::Address;
  using primitives::tipset::TipsetKey;
  using primitives::sector::Proof;

  outcome::result<void> checkPieces(const SectorInfo &sector_info,
                                    const std::shared_ptr<Api> &api);

  /**
   * Checks that data commitment generated in the sealing process matches
   * pieces, and that the seal ticket isn't expired
   */
  outcome::result<void> checkPrecommit(const Address &miner_address,
                                       const SectorInfo &sector_info,
                                       const TipsetKey &tipset_key,
                                       const ChainEpoch &height,
                                       const std::shared_ptr<Api> &api);

  outcome::result<void> checkCommit(const Address &miner_address,
                                    const SectorInfo &sector_info,
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
    kBadSeed,
    kPrecommitNotFound,
    kBadSealedCid,
    kInvalidProof,
  };

}  // namespace fc::sector_storage::checks

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::checks, ChecksError);

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_CHECKS_CHECKS_HPP
