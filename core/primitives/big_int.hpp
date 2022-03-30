/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/multiprecision/cpp_int.hpp>

#include "codec/cbor/streams_annotation.hpp"
#include "codec/json/coding.hpp"

namespace fc::primitives {
  using BigInt = boost::multiprecision::cpp_int;
}  // namespace fc::primitives

namespace boost::multiprecision {
  JSON_ENCODE(cpp_int) {
    return fc::codec::json::encode(
                                   boost::lexical_cast<std::string>(v),allocator);
  }

  JSON_DECODE(cpp_int) {
    v = cpp_int{fc::codec::json::AsString(j)};
  }

  CBOR_ENCODE(cpp_int, big_int) {
    std::vector<uint8_t> bytes;
    if (big_int != 0) {
      bytes.push_back(big_int < 0 ? 1 : 0);
      export_bits(big_int, std::back_inserter(bytes), 8);
    }
    return s << bytes;
  }

  CBOR_DECODE(cpp_int, big_int) {
    std::vector<uint8_t> bytes;
    s >> bytes;
    if (bytes.empty()) {
      big_int = 0;
    } else {
      import_bits(big_int, bytes.begin() + 1, bytes.end());
      if (bytes[0] == 1) {
        big_int = -big_int;
      }
    }
    return s;
  }
}  // namespace boost::multiprecision
