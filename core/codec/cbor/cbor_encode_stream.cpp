/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_encode_stream.hpp"

#include "codec/cbor/cbor_errors.hpp"
#include "common/cmp.hpp"

namespace fc::codec::cbor {
  CborEncodeStream &CborEncodeStream::operator<<(const Bytes &bytes) {
    return *this << gsl::make_span(bytes);
  }

  CborEncodeStream &CborEncodeStream::operator<<(BytesIn bytes) {
    addCount(1);
    writeBytes(data_, bytes.size());
    append(data_, bytes);
    return *this;
  }

  CborEncodeStream &CborEncodeStream::operator<<(std::string_view str) {
    addCount(1);
    writeStr(data_, str.size());
    append(data_, common::span::cbytes(str));
    return *this;
  }

  CborEncodeStream &CborEncodeStream::operator<<(const CID &cid) {
    addCount(1);
    auto maybe_cid_bytes = cid.toBytes();
    if (maybe_cid_bytes.has_error()) {
      outcome::raise(CborEncodeError::kInvalidCID);
    }
    auto &bytes{maybe_cid_bytes.value()};
    writeCid(data_, bytes.size());
    append(data_, bytes);
    return *this;
  }

  CborEncodeStream &CborEncodeStream::operator<<(
      const CborEncodeStream &other) {
    addCount(other.is_list_ ? 1 : other.count_);
    if (other.is_list_) {
      writeList(data_, other.count_);
    }
    append(data_, other.data_);
    return *this;
  }

  struct LessCborKey {
    bool operator()(BytesIn lhs, BytesIn rhs) const {
      return less(lhs.size(), rhs.size(), lhs, rhs);
    }
  };

  CborEncodeStream &CborEncodeStream::operator<<(
      const std::map<std::string, CborEncodeStream> &map) {
    addCount(1);
    writeMap(data_, map.size());
    std::vector<std::pair<BytesIn, const CborEncodeStream *>> sorted;
    sorted.reserve(map.size());
    for (const auto &pair : map) {
      if (pair.second.count_ != 1) {
        outcome::raise(CborEncodeError::kExpectedMapValueSingle);
      }
      sorted.emplace_back(common::span::cbytes(pair.first), &pair.second);
    }
    std::sort(sorted.begin(), sorted.end(), [](auto &l, auto &r) {
      return LessCborKey{}(l.first, r.first);
    });
    auto count{count_};
    for (const auto &[key, value] : sorted) {
      *this << common::span::bytestr(key);
      *this << *value;
    }
    count_ = count;
    return *this;
  }

  CborEncodeStream &CborEncodeStream::operator<<(std::nullptr_t) {
    addCount(1);
    writeNull(data_);
    return *this;
  }

  Bytes CborEncodeStream::data() const {
    Bytes result;
    if (is_list_) {
      writeList(result, count_);
    }
    append(result, data_);
    return result;
  }

  CborEncodeStream CborEncodeStream::list() {
    CborEncodeStream stream;
    stream.is_list_ = true;
    return stream;
  }

  std::map<std::string, CborEncodeStream> CborEncodeStream::map() {
    return {};
  }

  CborEncodeStream CborEncodeStream::wrap(BytesIn data, size_t count) {
    CborEncodeStream s;
    copy(s.data_, data);
    s.count_ = count;
    return s;
  }

  void CborEncodeStream::addCount(size_t count) {
    count_ += count;
  }
}  // namespace fc::codec::cbor
