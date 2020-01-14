/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_DECODE_STREAM_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_DECODE_STREAM_HPP

#include "codec/cbor/cbor_common.hpp"

#include <vector>

#include <cbor.h>
#include <gsl/span>

namespace fc::codec::cbor {
  /** Decodes CBOR */
  class CborDecodeStream {
   public:
    static constexpr auto is_cbor_decoder_stream = true;

    explicit CborDecodeStream(gsl::span<const uint8_t> data);

    /** Decodes integer or bool */
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    CborDecodeStream &operator>>(T &num) {
      if constexpr (std::is_same_v<T, bool>) {
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
        if constexpr (std::is_unsigned_v<T>) {
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
          if (num64 > static_cast<int64_t>(std::numeric_limits<T>::max())
              || num64 < static_cast<int64_t>(std::numeric_limits<T>::min())) {
            outcome::raise(CborDecodeError::INT_OVERFLOW);
          }
          num = static_cast<T>(num64);
        }
      }
      next();
      return *this;
    }

    template <typename T>
    CborDecodeStream &operator>>(boost::optional<T> &optional) {
      if (isNull()) {
        optional = boost::none;
        next();
      } else {
        T value;
        *this >> value;
        optional = value;
      }
      return *this;
    }

    /** Decodes bytes */
    CborDecodeStream &operator>>(std::vector<uint8_t> &bytes);
    /** Decodes string */
    CborDecodeStream &operator>>(std::string &str);
    /** Decodes CID */
    CborDecodeStream &operator>>(CID &cid);
    /** Creates list container decode substream */
    CborDecodeStream list();
    /** Skips current element */
    void next();
    /** Checks if current element is CID */
    bool isCid() const;
    /** Checks if current element is list container */
    bool isList() const;
    /** Checks if current element is map container */
    bool isMap() const;
    bool isNull() const;
    /** Returns count of items in current element list container */
    size_t listLength() const;
    /** Returns CBOR bytes of current element */
    std::vector<uint8_t> raw() const;
    /** Creates map container decode substream map */
    std::map<std::string, CborDecodeStream> map();

   private:
    CborDecodeStream container() const;

    std::shared_ptr<std::vector<uint8_t>> data_;
    std::shared_ptr<CborParser> parser_;
    CborValue value_{};
  };
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_DECODE_STREAM_HPP
