/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_CODEC_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_CODEC_HPP

#include <string>
#include <vector>

#include "address.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "common/outcome.hpp"
#include "common/outcome_throw.hpp"

namespace fc::primitives::address {

  /**
   * @brief Encodes an Address to an array of bytes
   */
  std::vector<uint8_t> encode(const Address &address) noexcept;

  /**
   * @brief Decodes an Address from an array of bytes
   */
  outcome::result<Address> decode(const std::vector<uint8_t> &v);

  /**
   * @brief Encodes an Address to a string
   */
  std::string encodeToString(const Address &address);

  /**
   * @brief Decodes an Address from a string
   */
  outcome::result<Address> decodeFromString(const std::string &s);

  /**
   * @brief Encodes an Address to an array of bytes with string representation
   */

  std::string encodeToByteString(const Address &address);
  /**
   * @brief Decodes Address from an array of bytes with string representation
   */
  outcome::result<Address> decodeFromByteString(const std::string &s);

  CBOR_ENCODE(Address, address) {
    return s << encode(address);
  }

  CBOR_DECODE(Address, address) {
    std::vector<uint8_t> data{};
    s >> data;
    OUTCOME_EXCEPT(decoded, decode(data));
    address = std::move(decoded);
    return s;
  }

  /**
   * @brief A helper function that calculates a checksum of an Address protocol
   * + payload
   */
  std::vector<uint8_t> checksum(const Address &address);

  /**
   * @brief Validates whether the Address' checksum matches the provided expect
   */
  bool validateChecksum(const Address &address,
                        const std::vector<uint8_t> &expect);

};  // namespace fc::primitives::address

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_ADDRESS_CODEC_HPP
