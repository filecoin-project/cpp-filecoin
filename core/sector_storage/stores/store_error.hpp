/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORE_ERROR_HPP
#define CPP_FILECOIN_STORE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::sector_storage::stores {
  enum class StoreErrors {
    FindAndAllocate = 1,
    NotFoundPath,
  };
}  // namespace fc::sector_storage::stores

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::stores, StoreErrors);

#endif  // CPP_FILECOIN_STORE_ERROR_HPP
