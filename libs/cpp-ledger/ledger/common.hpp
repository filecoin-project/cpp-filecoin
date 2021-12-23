/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ledger {
  using Byte = uint8_t;
  using Bytes = std::vector<Byte>;
  using Error = std::optional<std::string>;

  constexpr uint16_t kVendorLedger = 0x2c97;
  constexpr uint16_t kUsagePageLedgerNanoS = 0xffa0;
  constexpr uint16_t kChannel = 0x0101;
  constexpr int kPacketSize = 64;

  // list of supported product ids as well as their corresponding interfaces
  const std::map<uint16_t, int> kSupportedLedgerProductId{
      {0x4011, 0},  // Ledger Nano X
      {0x1011, 0},  // Ledger Nano S
  };

}  // namespace ledger
