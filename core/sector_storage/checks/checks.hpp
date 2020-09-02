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
  using mining::SectorInfo;

  outcome::result<void> checkPieces(const SectorInfo &sector_info,
                                    const std::shared_ptr<Api> &api);

  enum class ChecksError {
    kInvalidDeal = 1,
    kExpiredDeal,
  };

}  // namespace fc::sector_storage::checks

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::checks, ChecksError);

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_CHECKS_CHECKS_HPP
