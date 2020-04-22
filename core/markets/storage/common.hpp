/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_COMMON_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_COMMON_HPP

#include "primitives/types.hpp"

namespace fc::markets::storage {

  using primitives::TokenAmount;

  struct Balance {
    TokenAmount locked;
    TokenAmount available;
  };

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_MARKETS_STORAGE_COMMON_HPP
