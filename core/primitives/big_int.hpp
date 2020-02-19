/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_BIG_INT_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_BIG_INT_HPP

#include <boost/multiprecision/cpp_int.hpp>

#include "codec/cbor/streams_annotation.hpp"

namespace fc::primitives {
  using BigInt = boost::multiprecision::cpp_int;
}  // namespace fc::primitives

namespace boost::multiprecision {
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

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_BIG_INT_HPP
