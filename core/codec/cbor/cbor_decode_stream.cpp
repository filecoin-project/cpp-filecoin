/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_decode_stream.hpp"

namespace fc::codec::cbor {
  CborDecodeStream::CborDecodeStream(BytesIn data) : partial{data} {
    readToken();
  }

  CborDecodeStream &CborDecodeStream::operator>>(BytesOut bytes) {
    auto size = bytesLength();
    if (static_cast<size_t>(bytes.size()) != size) {
      outcome::raise(CborDecodeError::kWrongSize);
    }
    BytesIn _bytes;
    if (!codec::read(_bytes, partial, size)) {
      outcome::raise(CborDecodeError::kInvalidCbor);
    }
    std::copy(_bytes.begin(), _bytes.end(), bytes.begin());
    readToken();
    return *this;
  }

  CborDecodeStream &CborDecodeStream::operator>>(Bytes &bytes) {
    bytes.resize(bytesLength());
    return *this >> gsl::make_span(bytes);
  }

  CborDecodeStream &CborDecodeStream::operator>>(std::string &str) {
    BytesIn bytes;
    if (!codec::read(bytes, partial, _as(token.strSize()))) {
      outcome::raise(CborDecodeError::kInvalidCbor);
    }
    str = common::span::bytestr(bytes);
    readToken();
    return *this;
  }

  CborDecodeStream &CborDecodeStream::operator>>(CID &cid) {
    BytesIn bytes;
    if (!codec::read(bytes, partial, _as(token.cidSize()))) {
      outcome::raise(CborDecodeError::kInvalidCbor);
    }
    auto maybe_cid = CID::fromBytes(bytes);
    if (maybe_cid.has_error()) {
      outcome::raise(CborDecodeError::kInvalidCID);
    }
    cid = std::move(maybe_cid.value());
    readToken();
    return *this;
  }

  CborDecodeStream CborDecodeStream::list() {
    listLength();
    CborDecodeStream stream{readNested()};
    stream.readToken();
    return stream;
  }

  std::map<std::string, CborDecodeStream> CborDecodeStream::map() {
    const auto n{_as(token.mapCount())};
    readToken();
    std::map<std::string, CborDecodeStream> map;
    std::string key;
    for (size_t i{}; i < n; ++i) {
      *this >> key;
      map.emplace(key, CborDecodeStream{readNested()});
    }
    return map;
  }

  CborDecodeStream &CborDecodeStream::named(
      std::map<std::string, CborDecodeStream> &map, const std::string &name) {
    auto it{map.find(name)};
    if (it == map.end()) {
      outcome::raise(CborDecodeError::kKeyNotFound);
    }
    return it->second;
  }
}  // namespace fc::codec::cbor
