/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/optional.hpp>

#include "cbor_blake/cid_block.hpp"
#include "codec/cbor/cbor_errors.hpp"
#include "codec/cbor/cbor_token.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::codec::cbor {
  /** Decodes CBOR */
  class CborDecodeStream {
   public:
    static constexpr auto is_cbor_decoder_stream = true;

    explicit CborDecodeStream(BytesIn data);

    /** Decodes integer or bool */
    template <
        typename T,
        typename = std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
    CborDecodeStream &operator>>(T &num) {
      if constexpr (std::is_enum_v<T>) {
        std::underlying_type_t<T> value;
        *this >> value;
        num = static_cast<T>(value);
        return *this;
      }
      if constexpr (std::is_same_v<T, bool>) {
        num = _as(token.asBool());
      } else {
        if constexpr (std::is_unsigned_v<T>) {
          if (token.type == CborToken::Type::INT) {
            outcome::raise(CborDecodeError::kIntOverflow);
          }
          const auto num64{_as(token.asUint())};
          if (num64 > std::numeric_limits<T>::max()) {
            outcome::raise(CborDecodeError::kIntOverflow);
          }
          num = static_cast<T>(num64);
        } else {
          const auto num64{_as(token.asInt())};
          if (num64 > static_cast<int64_t>(std::numeric_limits<T>::max())
              || num64 < static_cast<int64_t>(std::numeric_limits<T>::min())) {
            outcome::raise(CborDecodeError::kIntOverflow);
          }
          num = static_cast<T>(num64);
        }
      }
      readToken();
      return *this;
    }

    /// Decodes nullable optional value
    template <typename T>
    CborDecodeStream &operator>>(boost::optional<T> &optional) {
      if (isNull()) {
        optional = boost::none;
        readToken();
      } else {
        T value{kDefaultT<T>()};
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
        T value{kDefaultT<T>()};
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
    CborDecodeStream &operator>>(BytesOut bytes);
    /** Decodes bytes */
    CborDecodeStream &operator>>(Bytes &bytes);
    /** Decodes string */
    CborDecodeStream &operator>>(std::string &str);
    /** Decodes CID */
    CborDecodeStream &operator>>(CID &cid);
    CborDecodeStream &operator>>(CbCid &cid);
    CborDecodeStream &operator>>(BlockParentCbCids &parents);
    /** Creates list container decode substream */
    CborDecodeStream list();
    /** Skips current element */
    void next() {
      readNested();
    }
    /** Checks if current element is CID */
    bool isCid() const {
      return (bool)token.cidSize();
    }
    /** Checks if current element is list container */
    bool isList() const {
      return (bool)token.listCount();
    }
    /** Checks if current element is map container */
    bool isMap() const {
      return (bool)token.mapCount();
    }
    bool isNull() const {
      return token.isNull();
    }
    bool isBool() const {
      return (bool)token.asBool();
    }
    bool isInt() const {
      return token.asInt() || token.asUint();
    }
    bool isStr() const {
      return (bool)token.strSize();
    }
    bool isBytes() const {
      return (bool)token.bytesSize();
    }

    /** Returns count of items in current element list container */
    size_t listLength() const {
      return _as(token.listCount());
    }
    /** Reads CBOR bytes of current element (and advances to the next element)
     */
    Bytes raw() {
      return copy(readNested());
    }
    /** Creates map container decode substream map */
    std::map<std::string, CborDecodeStream> map();
    static CborDecodeStream &named(std::map<std::string, CborDecodeStream> &map,
                                   const std::string &name);
    /// Returns bytestring length
    size_t bytesLength() const {
      return _as(token.bytesSize());
    }

    template <typename T>
    auto get() {
      T v{};
      *this >> v;
      return v;
    }

   private:
    template <typename T>
    T _as(const std::optional<T> &opt) const {
      if (!token) {
        outcome::raise(CborDecodeError::kInvalidCbor);
      }
      if (!opt) {
        outcome::raise(CborDecodeError::kWrongType);
      }
      return *opt;
    }
    void readToken() {
      input = partial;
      if (!partial.empty() && !read(token, partial)) {
        outcome::raise(CborDecodeError::kInvalidCbor);
      }
    }
    BytesIn readNested() {
      BytesIn raw;
      if (!cbor::readNested(raw, input)) {
        outcome::raise(CborDecodeError::kInvalidCbor);
      }
      partial = input;
      readToken();
      return raw;
    }

    BytesIn partial;
    BytesIn input;
    CborToken token;
  };
}  // namespace fc::codec::cbor
