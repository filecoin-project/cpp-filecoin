/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_TYPES_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_TYPES_HPP

#include "primitives/types.hpp"

namespace fc::markets::retrieval::provider {
  using primitives::TokenAmount;

  /**
   * @struct Provider config
   */
  struct ProviderConfig {
    TokenAmount price_per_byte;
    uint64_t payment_interval;
    uint64_t interval_increase;
  };

}  // namespace fc::markets::retrieval::provider

#endif
