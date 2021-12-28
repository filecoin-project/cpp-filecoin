/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ledger {

  using Byte = uint8_t;
  using Bytes = std::vector<Byte>;
  using Error = std::optional<std::string>;

}  // namespace ledger
