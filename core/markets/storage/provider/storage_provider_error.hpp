/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::markets::storage::provider {

  enum class StorageProviderError {
    kProviderStartError = 1,
    kStreamLookupError,
    kNotFoundSector,
  };

}  // namespace fc::markets::storage::provider

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::provider, StorageProviderError);
