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

#include "codec/cbor/streams_annotation.hpp"
#include "common/hexutil.hpp"
#include "common/buffer.hpp"

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
      if constexpr (std::is_enum_v<T>) {
        std::underlying_type_t<T> value;
        *this >> value;
        num = static_cast<T>(value);
        return *this;
      }
      if constexpr (std::is_same_v<T, bool>) {
        if (!cbor_value_is_boolean(&value_)) {
          outcome::raise(CborDecodeError::kWrongType);
        }
        bool bool_value;
        cbor_value_get_boolean(&value_, &bool_value);
        num = bool_value;
      } else {
        if (!cbor_value_is_integer(&value_)) {
          outcome::raise(CborDecodeError::kWrongType);
        }
        if constexpr (std::is_unsigned_v<T>) {
          if (!cbor_value_is_unsigned_integer(&value_)) {
            outcome::raise(CborDecodeError::kIntOverflow);
          }
          uint64_t num64;
          cbor_value_get_uint64(&value_, &num64);
          if (num64 > std::numeric_limits<T>::max()) {
            outcome::raise(CborDecodeError::kIntOverflow);
          }
          num = static_cast<T>(num64);
        } else {
          int64_t num64;
          cbor_value_get_int64(&value_, &num64);
          if (num64 > static_cast<int64_t>(std::numeric_limits<T>::max())
              || num64 < static_cast<int64_t>(std::numeric_limits<T>::min())) {
            outcome::raise(CborDecodeError::kIntOverflow);
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
    bool isBool() const;
    bool isInt() const;
    bool isStr() const;
    bool isBytes() const;

    // no more bytes to decode
    bool isEOF() const;

    /** Returns count of items in current element list container */
    size_t listLength() const;
    /** Reads CBOR bytes of current element (and advances to the next element)
     */
    std::vector<uint8_t> raw();
    /** Creates map container decode substream map */
    std::map<std::string, CborDecodeStream> map();
    /// Returns bytestring length
    size_t bytesLength() const;

    template <typename T>
    auto get() {
      T v{};
      *this >> v;
      return v;
    }

   private:
    CborDecodeStream container() const;

    std::shared_ptr<std::vector<uint8_t>> data_;
    std::shared_ptr<CborParser> parser_;
    CborValue value_{};
  };

  /*
  namespace {

    inline std::string dH(BytesIn b) {
      return common::hex_lower(b);
    }
    inline std::string dC(const CID &c) {
      auto &mh{c.content_address}; auto h{mh.getHash()};
      if (c.version == CID::Version::V1 && c.content_type == CID::Multicodec::DAG_CBOR && mh.getType() == libp2p::multi::HashType::blake2b_256) {
        return dH(h);
      }
      OUTCOME_EXCEPT(p, c.toBytes());
      return dH(gsl::make_span(p).subspan(0, p.size() - mh.toBuffer().size())) + "_" + dH(h);
    }
    struct LessCborKey {
      bool operator()(const std::string &l, const std::string &r) const {
        if (ssize_t diff = l.size() - r.size()) {
          return diff < 0;
        }
        return memcmp(l.data(), r.data(), l.size()) < 0;
      }
    };
    inline void dCb(std::string &o, codec::cbor::CborDecodeStream &s) {
      auto _s{[&](auto &s) { o += "^" + s; }};
      if (s.isCid()) {
        CID c; s >> c; o += "@" + dC(c);
      } else if (s.isList()) {
        o += "[";
        auto n{s.listLength()}; auto l{s.list()};
        for (auto i{0u}; i < n; ++i) {
          if (i) o += ",";
          dCb(o, l);
        }
        o += "]";
      } else if (s.isMap()) {
        o += "{";
        auto c{false};
        auto _m{s.map()};
        std::map<std::string, codec::cbor::CborDecodeStream, LessCborKey> m{_m.begin(), _m.end()};
        for (auto &p : m) {
          if (c) o += ","; else c = true;
          _s(p.first); o += ":";
          dCb(o, p.second);
        }
        o += "}";
      } else if (s.isBytes()) {
        Buffer b; s >> b; if (b.empty()) o += "~"; else o += dH(b);
      } else if (s.isStr()) {
        std::string b; s >> b; _s(b);
      } else if (s.isInt()) {
        int64_t i; s >> i; if (i >= 0) o += "+"; o += std::to_string(i);
      } else if (s.isBool()) {
        bool b; s >> b; o += b ? "T" : "F";
      } else if (s.isNull()) {
        o += "N"; s.next();
      } else {
        assert(false);
      }
    }
    inline std::string dCb(BytesIn b) {
      if (b.empty()) { return "(empty)"; }
      try {
        std::string o; codec::cbor::CborDecodeStream s{b}; dCb(o, s); return o;
      } catch (std::system_error &) {
        return "(error:" + dH(b) + ")";
      }
    }

  }
*/
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_DECODE_STREAM_HPP
