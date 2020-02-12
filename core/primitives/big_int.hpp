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

  struct UBigInt : cpp_int,
                   boost::totally_ordered<UBigInt>,
                   boost::arithmetic<UBigInt>,
                   boost::totally_ordered<UBigInt, int> {
    using cpp_int::cpp_int;

    inline bool operator==(int other) const {
      return static_cast<cpp_int>(*this) == other;
    }

    inline bool operator==(const UBigInt &other) const {
      return static_cast<cpp_int>(*this) == static_cast<cpp_int>(other);
    }

    inline bool operator<(int other) const {
      return static_cast<cpp_int>(*this) < other;
    }

    inline bool operator<(const UBigInt &other) const {
      return static_cast<cpp_int>(*this) < static_cast<cpp_int>(other);
    }

    inline UBigInt operator+=(const UBigInt &other) {
      static_cast<cpp_int *>(this)->operator+=(static_cast<cpp_int>(other));
      return *this;
    }

    inline UBigInt operator-=(const UBigInt &other) {
      static_cast<cpp_int *>(this)->operator-=(static_cast<cpp_int>(other));
      return *this;
    }

    inline UBigInt operator*=(const UBigInt &other) {
      static_cast<cpp_int *>(this)->operator*=(static_cast<cpp_int>(other));
      return *this;
    }

    inline UBigInt operator/=(const UBigInt &other) {
      static_cast<cpp_int *>(this)->operator/=(static_cast<cpp_int>(other));
      return *this;
    }

    inline UBigInt operator-(int other) const {
      return static_cast<cpp_int>(*this) - other;
    }
  };

  struct BigInt : cpp_int,
                  boost::totally_ordered<BigInt>,
                  boost::totally_ordered<BigInt, int>,
                  boost::arithmetic<BigInt>,
                  boost::multiplicative<BigInt, unsigned long>,
                  boost::less_than_comparable<BigInt, UBigInt>,
                  boost::multipliable<BigInt, UBigInt> {
    using cpp_int::cpp_int;

    inline bool operator==(int other) const {
      return static_cast<cpp_int>(*this) == other;
    }

    inline bool operator<(int other) const {
      return static_cast<cpp_int>(*this) < other;
    }

    inline bool operator==(const BigInt &other) const {
      return static_cast<cpp_int>(*this) == static_cast<cpp_int>(other);
    }

    inline bool operator<(const BigInt &other) const {
      return static_cast<cpp_int>(*this) < static_cast<cpp_int>(other);
    }

    inline BigInt operator+=(const BigInt &other) {
      static_cast<cpp_int *>(this)->operator+=(static_cast<cpp_int>(other));
      return *this;
    }

    inline BigInt operator-=(const BigInt &other) {
      static_cast<cpp_int *>(this)->operator-=(static_cast<cpp_int>(other));
      return *this;
    }

    inline BigInt operator*=(const BigInt &other) {
      static_cast<cpp_int *>(this)->operator*=(static_cast<cpp_int>(other));
      return *this;
    }

    inline BigInt operator/=(const BigInt &other) {
      static_cast<cpp_int *>(this)->operator/=(static_cast<cpp_int>(other));
      return *this;
    }

    inline BigInt operator*=(const unsigned long &other) {
      static_cast<cpp_int *>(this)->operator*=(other);
      return *this;
    }

    inline BigInt operator/=(const unsigned long &other) {
      static_cast<cpp_int *>(this)->operator/=(other);
      return *this;
    }

    inline bool operator<(const UBigInt &other) const {
      return static_cast<cpp_int>(*this) < static_cast<cpp_int>(other);
    }

    inline bool operator>(const UBigInt &other) const {
      return static_cast<cpp_int>(*this) > static_cast<cpp_int>(other);
    }

    inline BigInt operator*=(const UBigInt &other) {
      static_cast<cpp_int *>(this)->operator*=(static_cast<cpp_int>(other));
      return *this;
    }
  };

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

  template <class Stream,
            class T,
            typename = std::enable_if_t<
                (std::is_same_v<T, BigInt> || std::is_same_v<T, UBigInt>)&&std::
                    remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, T &big_int) {
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
