/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_encode_stream.hpp"

namespace fc::codec::cbor {
  CborEncodeStream::CborEncodeStream(CborStreamType type) : type_(type) {}

  CborEncodeStream &CborEncodeStream::operator<<(
      const CborEncodeStream &other) {
    switch (other.type_) {
      case CborStreamType::SINGLE:
      case CborStreamType::LIST:
        addCount(1);
        break;
      case CborStreamType::FLAT:
        addCount(other.count_);
        break;
    }
    auto other_data = other.data();
    data_.insert(data_.end(), other_data.begin(), other_data.end());
    return *this;
  }

  std::vector<uint8_t> CborEncodeStream::data() const {
    switch (type_) {
      case CborStreamType::SINGLE:
      case CborStreamType::FLAT:
        return data_;
      case CborStreamType::LIST:
        return list();
    }
  }

  std::vector<uint8_t> CborEncodeStream::list() const {
    std::array<uint8_t, 9> prefix{0};
    CborEncoder encoder;
    CborEncoder container;
    cbor_encoder_init(&encoder, prefix.data(), prefix.size(), 0);
    cbor_encoder_create_array(&encoder, &container, count_);
    std::vector<uint8_t> result(
        prefix.begin(),
        prefix.begin()
            + cbor_encoder_get_buffer_size(&container, prefix.data()));
    result.insert(result.end(), data_.begin(), data_.end());
    return result;
  }

  void CborEncodeStream::addCount(size_t count) {
    if (type_ == CborStreamType::SINGLE && count_ + count != 1) {
    }
    count_ += count;
  }
}  // namespace fc::codec::cbor
