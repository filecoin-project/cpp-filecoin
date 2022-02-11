/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/address/address.hpp"

namespace fc::api {
  using primitives::address::Address;

  struct LedgerKeyInfo {
    Address address;
    std::vector<uint32_t> path;
  };

}  // namespace fc::api
