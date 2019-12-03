/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_DECODE_STREAM_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_DECODE_STREAM_HPP

#include "codec/cbor/cbor_common.hpp"

#include <cbor.h>
#include <gsl/span>
#include <vector>

namespace fc::codec::cbor {
  class CborDecodeStream {
   public:
    explicit CborDecodeStream(gsl::span<const uint8_t> data);

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    CborDecodeStream &operator>>(T &num) {
      if (std::is_same_v<T, bool>) {
        if (!cbor_value_is_boolean(&value_)) {
        }
        bool bool_value;
        cbor_value_get_boolean(&value_, &bool_value);
        num = bool_value;
      } else {
        if (!cbor_value_is_integer(&value_)) {
        }
        if (std::is_unsigned_v<T>) {
          if (!cbor_value_is_unsigned_integer(&value_)) {
          }
          uint64_t num64;
          cbor_value_get_uint64(&value_, &num64);
          if (num64 > std::numeric_limits<T>::max()) {
          }
          num = static_cast<T>(num64);
        } else {
          int64_t num64;
          cbor_value_get_int64(&value_, &num64);
          if (num64 > std::numeric_limits<T>::max() || num64 < std::numeric_limits<T>::min()) {
          }
          num = static_cast<T>(num64);
        }
      }
      cbor_value_advance(&value_);
      return *this;
    }

    CborDecodeStream &operator>>(libp2p::multi::ContentIdentifier &cid);

   private:
    CborStreamType type_;
    std::vector<uint8_t> data_;
    CborParser parser_{};
    CborValue value_{};
  };
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_DECODE_STREAM_HPP
