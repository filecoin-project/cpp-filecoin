/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "primitives/address/address.hpp"

namespace fc::primitives::address {

  /**
   * @brief Encodes an Address to an array of bytes
   */
  Buffer encode(const Address &address) noexcept;

  /**
   * @brief Decodes an Address from an array of bytes
   */
  outcome::result<Address> decode(BytesIn v);

  /**
   * @brief Encodes an Address to a string
   */
  std::string encodeToString(const Address &address);

  /**
   * @brief Decodes an Address from a string
   */
  outcome::result<Address> decodeFromString(const std::string &s);

  CBOR_ENCODE(Address, address) {
    return s << encode(address);
  }

  CBOR_DECODE(Address, address) {
    address = decode(s.template get<Buffer>()).value();
    return s;
  }

  /**
   * @brief A helper function that calculates a checksum of an Address protocol
   * + payload
   */
  Buffer checksum(const Address &address);
}  // namespace fc::primitives::address
