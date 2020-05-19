/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/storage_provider_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::provider,
                            StorageProviderError,
                            e) {
  using E = fc::markets::storage::provider::StorageProviderError;
  switch (e) {
    case E::PROVIDER_START_ERROR:
      return "StorageProviderError: cannot start provider";
    default:
      return "StorageProviderError: unknown error";
  }
}
