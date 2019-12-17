/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/rle/rle_plus_encoding_stream.hpp"

namespace fc::codec::rle {
  void RLEPlusEncodingStream::initContent() {
    content_.clear();
    content_.push_back(false);
    content_.push_back(false);
  }

  void RLEPlusEncodingStream::pushByte(uint8_t byte) {
    for (size_t i = 0; i < BYTE_BITS_COUNT; ++i) {
      bool bit = static_cast<bool>((byte & (1 << i)) >> i);
      content_.push_back(bit);
    }
  }
}  // namespace fc::codec::rle
