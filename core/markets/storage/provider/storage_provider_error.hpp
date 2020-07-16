/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_PROVIDER_STORAGE_PROVIDER_ERROR_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_PROVIDER_STORAGE_PROVIDER_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::markets::storage::provider {

  enum class StorageProviderError {
    kProviderStartError = 1,
    kStreamLookupError,
  };

}  // namespace fc::markets::storage::provider

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::provider, StorageProviderError);


#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_PROVIDER_STORAGE_PROVIDER_ERROR_HPP
