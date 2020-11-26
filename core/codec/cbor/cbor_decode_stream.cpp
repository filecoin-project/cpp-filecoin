/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_decode_stream.hpp"

namespace fc::codec::cbor {
  CborDecodeStream::CborDecodeStream(gsl::span<const uint8_t> data)
      : data_(std::make_shared<std::vector<uint8_t>>(data.begin(), data.end())),
        parser_(std::make_shared<CborParser>()) {
    if (CborNoError
        != cbor_parser_init(
            data_->data(), data_->size(), 0, parser_.get(), &value_)) {
      if (!data.empty()) {
        outcome::raise(CborDecodeError::kInvalidCbor);
      }
    } else {
      value_.remaining = UINT32_MAX;
    }
  }

  CborDecodeStream &CborDecodeStream::operator>>(gsl::span<uint8_t> bytes) {
    auto size = bytesLength();
    if (static_cast<size_t>(bytes.size()) != size) {
      outcome::raise(CborDecodeError::kWrongSize);
    }
    auto value = value_;
    value.remaining = 1;
    if (CborNoError
        != cbor_value_copy_byte_string(&value, bytes.data(), &size, nullptr)) {
      outcome::raise(CborDecodeError::kInvalidCbor);
    }
    next();
    return *this;
  }

  CborDecodeStream &CborDecodeStream::operator>>(std::vector<uint8_t> &bytes) {
    bytes.resize(bytesLength());
    return *this >> gsl::make_span(bytes);
  }

  CborDecodeStream &CborDecodeStream::operator>>(std::string &str) {
    if (!cbor_value_is_text_string(&value_)) {
      outcome::raise(CborDecodeError::kWrongType);
    }
    size_t size;
    if (CborNoError != cbor_value_get_string_length(&value_, &size)) {
      outcome::raise(CborDecodeError::kInvalidCbor);
    }
    str.resize(size);
    auto value = value_;
    value.remaining = 1;
    if (CborNoError
        != cbor_value_copy_text_string(&value, str.data(), &size, nullptr)) {
      outcome::raise(CborDecodeError::kInvalidCbor);
    }
    next();
    return *this;
  }

  CborDecodeStream &CborDecodeStream::operator>>(CID &cid) {
    if (!cbor_value_is_tag(&value_)) {
      outcome::raise(CborDecodeError::kInvalidCborCID);
    }
    CborTag tag;
    cbor_value_get_tag(&value_, &tag);
    if (tag != kCidTag) {
      outcome::raise(CborDecodeError::kInvalidCborCID);
    }
    if (CborNoError != cbor_value_advance(&value_)) {
      outcome::raise(CborDecodeError::kInvalidCbor);
    }
    if (!cbor_value_is_byte_string(&value_)) {
      outcome::raise(CborDecodeError::kInvalidCborCID);
    }
    std::vector<uint8_t> bytes;
    *this >> bytes;
    if (bytes[0] != 0) {
      outcome::raise(CborDecodeError::kInvalidCborCID);
    }
    bytes.erase(bytes.begin());
    auto maybe_cid = CID::fromBytes(bytes);
    if (maybe_cid.has_error()) {
      outcome::raise(CborDecodeError::kInvalidCID);
    }
    cid = std::move(maybe_cid.value());
    return *this;
  }

  CborDecodeStream CborDecodeStream::list() {
    if (!isList()) {
      outcome::raise(CborDecodeError::kWrongType);
    }
    auto stream = container();
    next();
    return stream;
  }

  void CborDecodeStream::next() {
    if (isCid()) {
      if (CborNoError != cbor_value_skip_tag(&value_)) {
        outcome::raise(CborDecodeError::kInvalidCbor);
      }
    }
    auto remaining = value_.remaining;
    value_.remaining = 1;
    if (CborNoError != cbor_value_advance(&value_)) {
      outcome::raise(CborDecodeError::kInvalidCbor);
    }
    if (value_.ptr != parser_->end) {
      remaining += value_.remaining - 1;
      if (CborNoError
          != cbor_parser_init(value_.ptr,
                              parser_->end - value_.ptr,
                              0,
                              parser_.get(),
                              &value_)) {
        outcome::raise(CborDecodeError::kInvalidCbor);
      }
      value_.remaining = remaining;
    }
  }

  bool CborDecodeStream::isCid() const {
    if (!cbor_value_is_tag(&value_)) {
      return false;
    }
    CborTag tag;
    cbor_value_get_tag(&value_, &tag);
    return tag == kCidTag;
  }

  bool CborDecodeStream::isList() const {
    return cbor_value_is_array(&value_);
  }

  bool CborDecodeStream::isMap() const {
    return cbor_value_is_map(&value_);
  }

  bool CborDecodeStream::isNull() const {
    return cbor_value_is_null(&value_);
  }

  bool CborDecodeStream::isBool() const {
    return cbor_value_is_boolean(&value_);
  }

  bool CborDecodeStream::isInt() const {
    return cbor_value_is_integer(&value_);
  }

  bool CborDecodeStream::isStr() const {
    return cbor_value_is_text_string(&value_);
  }

  bool CborDecodeStream::isBytes() const {
    return cbor_value_is_byte_string(&value_);
  }

  bool CborDecodeStream::isEOF() const {
    return value_.ptr == value_.parser->end;
  }

  size_t CborDecodeStream::listLength() const {
    if (!isList()) {
      outcome::raise(CborDecodeError::kWrongType);
    }
    size_t length;
    if (CborNoError != cbor_value_get_array_length(&value_, &length)) {
      outcome::raise(CborDecodeError::kInvalidCbor);
    }
    return length;
  }

  std::vector<uint8_t> CborDecodeStream::raw() {
    auto begin = value_.ptr;
    next();
    return {begin, value_.ptr};
  }

  std::map<std::string, CborDecodeStream> CborDecodeStream::map() {
    if (!cbor_value_is_map(&value_)) {
      outcome::raise(CborDecodeError::kWrongType);
    }
    auto stream = container();
    next();
    std::map<std::string, CborDecodeStream> map;
    std::string key;
    const uint8_t *begin;
    while (!cbor_value_at_end(&stream.value_)) {
      stream >> key;
      begin = stream.value_.ptr;
      auto stream2 = stream;
      stream2.parser_ = std::make_shared<CborParser>();
      if (CborNoError != cbor_value_skip_tag(&stream.value_)) {
        outcome::raise(CborDecodeError::kInvalidCbor);
      }
      if (CborNoError != cbor_value_advance(&stream.value_)) {
        outcome::raise(CborDecodeError::kInvalidCbor);
      }
      if (CborNoError
          != cbor_parser_init(begin,
                              stream.value_.ptr - begin,
                              0,
                              stream2.parser_.get(),
                              &stream2.value_)) {
        outcome::raise(CborDecodeError::kInvalidCbor);
      }
      map.insert(std::make_pair(key, stream2));
    }
    return map;
  }

  size_t CborDecodeStream::bytesLength() const {
    if (!cbor_value_is_byte_string(&value_)) {
      outcome::raise(CborDecodeError::kWrongType);
    }
    size_t size;
    if (CborNoError != cbor_value_get_string_length(&value_, &size)) {
      outcome::raise(CborDecodeError::kInvalidCbor);
    }
    return size;
  }

  CborDecodeStream CborDecodeStream::container() const {
    auto stream = *this;
    if (CborNoError != cbor_value_enter_container(&value_, &stream.value_)) {
      outcome::raise(CborDecodeError::kInvalidCbor);
    }
    return stream;
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
