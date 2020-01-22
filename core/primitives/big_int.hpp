/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_BIG_INT_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_BIG_INT_HPP

#include <boost/multiprecision/cpp_int.hpp>

#include "codec/cbor/cbor.hpp"

namespace fc::primitives {
  using boost::multiprecision::cpp_int;

  struct BigInt : cpp_int {
    using cpp_int::cpp_int;
  };

  struct UBigInt : cpp_int {
    using cpp_int::cpp_int;
  };

  static inline BigInt operator*(const BigInt &lhs, const BigInt &rhs) {
    return static_cast<cpp_int>(lhs) * static_cast<cpp_int>(rhs);
  }

  static inline BigInt operator*(const unsigned long &lhs, const BigInt &rhs) {
    return static_cast<cpp_int>(lhs) * static_cast<cpp_int>(rhs);
  }

  static inline BigInt operator+(const BigInt &lhs, const BigInt &rhs) {
    return static_cast<cpp_int>(lhs) + static_cast<cpp_int>(rhs);
  }

  static inline BigInt operator-(const BigInt &lhs, const BigInt &rhs) {
    return static_cast<cpp_int>(lhs) - static_cast<cpp_int>(rhs);
  }

  static inline bool operator<(const BigInt &lhs, const BigInt &rhs) {
    return static_cast<cpp_int>(lhs) < static_cast<cpp_int>(rhs);
  }

  static inline bool operator<(const BigInt &lhs, int rhs) {
    return static_cast<cpp_int>(lhs) < rhs;
  }

  static inline bool operator==(const BigInt &lhs, const BigInt &rhs) {
    return static_cast<cpp_int>(lhs) == static_cast<cpp_int>(rhs);
  }

  static inline bool operator==(const BigInt &lhs, int rhs) {
    return static_cast<cpp_int>(lhs) == rhs;
  }

  template <class Stream,
            class T,
            typename = std::enable_if_t<
                (std::is_same_v<T, BigInt> || std::is_same_v<T, UBigInt>)&&std::
                    remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const T &big_int) {
    std::vector<uint8_t> bytes;
    if (big_int != 0) {
      if (std::is_same_v<T, BigInt>) {
        bytes.push_back(big_int < 0 ? 1 : 0);
      }
      export_bits(big_int, std::back_inserter(bytes), 8);
    }
    return s << bytes;
  }

  template <
      class Stream,
      class T,
      typename = std::enable_if_t<
          (std::is_same_v<T, BigInt> || std::is_same_v<T, UBigInt>)&&Stream::
              is_cbor_decoder_stream>>
  Stream &operator>>(Stream &s, T &big_int) {
    std::vector<uint8_t> bytes;
    s >> bytes;
    if (bytes.empty()) {
      big_int = 0;
    } else {
      import_bits(big_int,
                  bytes.begin() + (std::is_same_v<T, BigInt> ? 1 : 0),
                  bytes.end());
      if (std::is_same_v<T, BigInt> && bytes[0] == 1) {
        big_int = -big_int;
      }
    }
    return s;
  }
}  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_BIG_INT_HPP
