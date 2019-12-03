/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_decode_stream.hpp"

namespace fc::codec::cbor {
  CborDecodeStream::CborDecodeStream(gsl::span<const uint8_t> data)
      : type_(CborStreamType::FLAT),
        data_(
            std::make_shared<std::vector<uint8_t>>(data.begin(), data.end())) {
    if (CborNoError
        != cbor_parser_init(
            data_->data(), data_->size(), 0, &parser_, &value_)) {
      outcome::raise(CborDecodeError::INVALID_CBOR);
    }
    value_.remaining = UINT32_MAX;
  }

  CborDecodeStream &CborDecodeStream::operator>>(std::vector<uint8_t> &bytes) {
    if (!cbor_value_is_byte_string(&value_)) {
      outcome::raise(CborDecodeError::WRONG_TYPE);
    }
    size_t size;
    if (CborNoError != cbor_value_get_string_length(&value_, &size)) {
      outcome::raise(CborDecodeError::INVALID_CBOR);
    }
    bytes.resize(size);
    auto value = value_;
    value.remaining = 1;
    if (CborNoError != cbor_value_copy_byte_string(&value, bytes.data(), &size, nullptr)) {
      outcome::raise(CborDecodeError::INVALID_CBOR);
    }
    next();
    return *this;
  }

  CborDecodeStream &CborDecodeStream::operator>>(
      libp2p::multi::ContentIdentifier &cid) {
    if (!cbor_value_is_tag(&value_)) {
      outcome::raise(CborDecodeError::INVALID_CBOR_CID);
    }
    CborTag tag;
    cbor_value_get_tag(&value_, &tag);
    if (tag != kCidTag) {
      outcome::raise(CborDecodeError::INVALID_CBOR_CID);
    }
    if (CborNoError != cbor_value_advance(&value_)) {
      outcome::raise(CborDecodeError::INVALID_CBOR);
    }
    if (!cbor_value_is_byte_string(&value_)) {
      outcome::raise(CborDecodeError::INVALID_CBOR_CID);
    }
    std::vector<uint8_t> bytes;
    *this >> bytes;
    if (bytes[0] != 0) {
      outcome::raise(CborDecodeError::INVALID_CBOR_CID);
    }
    bytes.erase(bytes.begin());
    auto maybe_cid = libp2p::multi::ContentIdentifierCodec::decode(bytes);
    if (maybe_cid.has_error()) {
      outcome::raise(CborDecodeError::INVALID_CID);
    }
    cid = maybe_cid.value();
    return *this;
  }

  CborDecodeStream CborDecodeStream::list() {
    if (!cbor_value_is_array(&value_)) {
      outcome::raise(CborDecodeError::WRONG_TYPE);
    }
    auto stream = *this;
    stream.type_ = CborStreamType::LIST;
    stream.value_.parser = &stream.parser_;
    if (CborNoError != cbor_value_enter_container(&value_, &stream.value_)) {
      outcome::raise(CborDecodeError::INVALID_CBOR);
    }
    next();
    return stream;
  }

  void CborDecodeStream::next() {
    auto remaining = value_.remaining;
    if (type_ == CborStreamType::FLAT) {
      value_.remaining = 1;
    }
    if (CborNoError != cbor_value_advance(&value_)) {
      outcome::raise(CborDecodeError::INVALID_CBOR);
    }
    if (type_ == CborStreamType::FLAT && value_.ptr != parser_.end) {
      value_.remaining = remaining + value_.remaining - 1;
      if (CborNoError != cbor_parser_init(value_.ptr, parser_.end - value_.ptr, 0, &parser_, &value_)) {
        outcome::raise(CborDecodeError::INVALID_CBOR);
      }
    }
  }
}  // namespace fc::codec::cbor
