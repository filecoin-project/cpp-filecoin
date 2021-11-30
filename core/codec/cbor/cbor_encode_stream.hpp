/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>

#include <boost/optional.hpp>

#include "cbor_blake/cid_block.hpp"
#include "codec/cbor/cbor_token.hpp"
#include "common/enum.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::codec::cbor {
  class CborEncodeStream;

  /** Cbor map with respect for key order */
  struct CborOrderedMap
      : public std::vector<std::pair<std::string, CborEncodeStream>> {
    CborEncodeStream &operator[](const std::string &key);
  };

  /** Encodes CBOR */
  class CborEncodeStream {
   public:
    static constexpr auto is_cbor_encoder_stream = true;

    /** Encodes integer or bool */
    template <
        typename T,
        typename = std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
    CborEncodeStream &operator<<(T num) {
      if constexpr (std::is_enum_v<T>) {
        return *this << common::to_int(num);
      } else {
        addCount(1);
        if constexpr (std::is_same_v<T, bool>) {
          writeBool(data_, num);
        } else if constexpr (std::is_unsigned_v<T>) {
          writeUint(data_, num);
        } else {
          writeInt(data_, num);
        }
      }
      return *this;
    }

    /// Encodes nullable optional value
    template <typename T>
    CborEncodeStream &operator<<(const boost::optional<T> &optional) {
      if (optional) {
        *this << *optional;
      } else {
        *this << nullptr;
      }
      return *this;
    }

    /// Encodes elements into list
    template <typename T>
    CborEncodeStream &operator<<(const gsl::span<T> &values) {
      writeList(data_, values.size());
      ++count_;
      const auto count{count_};
      for (auto &value : values) {
        *this << value;
      }
      count_ = count;
      return *this;
    }

    /// Encodes elements into map
    template <typename T>
    CborEncodeStream &operator<<(const std::map<std::string, T> &items) {
      auto m = map();
      for (auto &item : items) {
        m[item.first] << item.second;
      }
      return *this << m;
    }

    /// Encodes vector into list
    template <typename T>
    CborEncodeStream &operator<<(const std::vector<T> &values) {
      return *this << gsl::make_span(values);
    }

    /// Encodes array into list
    template <class T, size_t size>
    CborEncodeStream &operator<<(const std::array<T, size> &values) {
      return *this << gsl::make_span(values);
    }

    /** Encodes bytes */
    CborEncodeStream &operator<<(const Bytes &bytes);
    /** Encodes bytes */
    CborEncodeStream &operator<<(BytesIn bytes);
    /** Encodes string */
    CborEncodeStream &operator<<(std::string_view str);
    /** Encodes CID */
    CborEncodeStream &operator<<(const CID &cid);
    CborEncodeStream &operator<<(const CbCid &cid);
    CborEncodeStream &operator<<(const BlockParentCbCids &parents);
    /** Encodes list container encode substream */
    CborEncodeStream &operator<<(const CborEncodeStream &other);
    /** Encodes map container encode substream map with canonical order */
    CborEncodeStream &operator<<(
        const std::map<std::string, CborEncodeStream> &map);
    /** Encodes map container encode substream map with order respect */
    CborEncodeStream &operator<<(const CborOrderedMap &map);
    /** Encodes null */
    CborEncodeStream &operator<<(std::nullptr_t);
    /** Returns CBOR bytes of encoded elements */
    Bytes data() const;
    /** Returns the number of elements */
    size_t count() const;
    /** Creates list container encode substream */
    static CborEncodeStream list();
    /** Creates map container encode substream map */
    static std::map<std::string, CborEncodeStream> map();
    /** Creates map container encode substream map */
    static CborOrderedMap orderedMap();
    /** Wraps CBOR bytes */
    static CborEncodeStream wrap(BytesIn data, size_t count);

   private:
    void addCount(size_t count);

    bool is_list_{false};
    Bytes data_{};
    size_t count_{0};
  };
}  // namespace fc::codec::cbor
