/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/uvarint.hpp"
#include "primitives/address/address.hpp"

namespace fc::codec::cbor::light_reader {
  using primitives::address::Protocol;

  /** Protocol indicator for ID-address */
  constexpr std::array<uint8_t, 1> kIdPrefix{Protocol::ID};

  /**
   * Decodes an ID-address
   * @param[out] id - an id stored in address
   * @param[in] input - bytes to decode
   * @return true on success, otherwise false
   */
  inline bool readIdAddress(uint64_t &id, BytesIn &input) {
    id = {};
    return readPrefix(input, kIdPrefix) && uvarint::read(id, input);
  }
}  // namespace fc::codec::cbor::light_reader
