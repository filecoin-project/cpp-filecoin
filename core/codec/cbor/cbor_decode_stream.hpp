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
    template <
        typename T,
        typename = std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
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

    /// Decodes nullable optional value
    template <typename T>
    CborDecodeStream &operator>>(boost::optional<T> &optional) {
      if (isNull()) {
        optional = boost::none;
        next();
      } else {
        T value{};
        *this >> value;
        optional = value;
      }
      return *this;
    }

    /// Decodes list elements into vector
    template <typename T>
    CborDecodeStream &operator>>(std::vector<T> &values) {
      auto n = listLength();
      auto l = list();
      values.clear();
      values.reserve(n);
      for (auto i = 0u; i < n; ++i) {
        T value{};
        l >> value;
        values.push_back(value);
      }
      return *this;
    }

    /// Decodes elements to map
    template <typename T>
    CborDecodeStream &operator>>(std::map<std::string, T> &items) {
      for (auto &m : map()) {
        m.second >> items[m.first];
      }
      return *this;
    }

    /// Decodes bytes
    CborDecodeStream &operator>>(gsl::span<uint8_t> bytes);
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
    /** Reads CBOR bytes of current element (and advances to the next element)
     */
    std::vector<uint8_t> raw();
    /** Creates map container decode substream map */
    std::map<std::string, CborDecodeStream> map();
    /// Returns bytestring length
    size_t bytesLength() const;

   private:
    CborDecodeStream container() const;

    std::shared_ptr<std::vector<uint8_t>> data_;
    std::shared_ptr<CborParser> parser_;
    CborValue value_{};
  };
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_DECODE_STREAM_HPP
