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
    cbor_parser_init(data_->data(), data_->size(), 0, &parser_, &value_);
    value_.remaining = UINT32_MAX;
  }

  CborDecodeStream &CborDecodeStream::operator>>(
      libp2p::multi::ContentIdentifier &cid) {
    if (!cbor_value_is_tag(&value_)) {
    }
    CborTag tag;
    cbor_value_get_tag(&value_, &tag);
    if (tag != kCidTag) {
    }
    if (CborNoError != cbor_value_advance(&value_)) {
    }
    if (!cbor_value_is_byte_string(&value_)) {
    }
    size_t bytes_length;
    if (CborNoError != cbor_value_get_string_length(&value_, &bytes_length)) {
    }
    std::vector<uint8_t> bytes(bytes_length);
    if (CborNoError
        != cbor_value_copy_byte_string(
            &value_, bytes.data(), &bytes_length, nullptr)) {
    }
    if (bytes[0] != 0) {
    }
    bytes.erase(bytes.begin());
    auto maybe_cid = libp2p::multi::ContentIdentifierCodec::decode(bytes);
    cid = maybe_cid.value();
    return *this;
  }

  CborDecodeStream CborDecodeStream::list() {
    if (!cbor_value_is_array(&value_)) {
    }
    auto stream = *this;
    stream.type_ = CborStreamType::LIST;
    stream.value_.parser = &stream.parser_;
    if (CborNoError != cbor_value_enter_container(&value_, &stream.value_)) {
    }
    if (CborNoError != cbor_value_advance(&value_)) {
    }
    return stream;
  }
}  // namespace fc::codec::cbor
