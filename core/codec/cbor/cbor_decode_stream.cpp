/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_decode_stream.hpp"

namespace fc::codec::cbor {
  CborDecodeStream::CborDecodeStream(gsl::span<const uint8_t> data) : data_(data.begin(), data.end()) {
    cbor_parser_init(data_.data(), data_.size(), 0, &parser_, &value_);
  }
}  // namespace fc::codec::cbor
