/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/validate/with.hpp"
#include "primitives/address/address_codec.hpp"

namespace fc::primitives::address {
  CLI_VALIDATE(Address) {
    return validateWith(out, values, [](const std::string &value) {
      return decodeFromString(value).value();
    });
  }
}  // namespace fc::primitives::address
