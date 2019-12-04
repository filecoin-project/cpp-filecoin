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
          outcome::raise(CborDecodeError::WRONG_TYPE);
        }
        bool bool_value;
        cbor_value_get_boolean(&value_, &bool_value);
        num = bool_value;
      } else {
        if (!cbor_value_is_integer(&value_)) {
          outcome::raise(CborDecodeError::WRONG_TYPE);
        }
        if (std::is_unsigned_v<T>) {
          if (!cbor_value_is_unsigned_integer(&value_)) {
            outcome::raise(CborDecodeError::INT_OVERFLOW);
          }
          uint64_t num64;
          cbor_value_get_uint64(&value_, &num64);
          if (num64 > std::numeric_limits<T>::max()) {
            outcome::raise(CborDecodeError::INT_OVERFLOW);
          }
          num = static_cast<T>(num64);
        } else {
          int64_t num64;
          cbor_value_get_int64(&value_, &num64);
          if (num64 > std::numeric_limits<T>::max() || num64 < std::numeric_limits<T>::min()) {
            outcome::raise(CborDecodeError::INT_OVERFLOW);
          }
          num = static_cast<T>(num64);
        }
      }
      next();
      return *this;
    }

    CborDecodeStream &operator>>(std::vector<uint8_t> &bytes);
    CborDecodeStream &operator>>(std::string &str);
    CborDecodeStream &operator>>(libp2p::multi::ContentIdentifier &cid);
    CborDecodeStream list();
    void next();
    bool isCid() const;
    bool isList() const;
    bool isMap() const;
    size_t listLength() const;
    std::vector<uint8_t> raw() const;
    std::map<std::string, CborDecodeStream> map();

   private:
    CborDecodeStream container() const;

    std::shared_ptr<std::vector<uint8_t>> data_;
    std::shared_ptr<CborParser> parser_;
    CborValue value_{};
  };
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_DECODE_STREAM_HPP
