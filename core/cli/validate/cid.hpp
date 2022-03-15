/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/validate/with.hpp"
#include "primitives/cid/cid.hpp"

namespace fc {
  CLI_VALIDATE(CID) {
    return validateWith(out, values, [](const std::string &value) {
      return CID::fromString(value).value();
    });
  }
}  // namespace fc
