/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/uvarint.hpp"

namespace fc::codec::address {
  constexpr std::array<uint8_t, 1> kIdPrefix{0x00};

  inline bool readId(uint64_t &id, BytesIn &input) {
    id = {};
    return readPrefix(input, kIdPrefix) && uvarint::read(id, input);
  }
}  // namespace fc::codec::address
