/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_encode_stream.hpp"

namespace fc::codec::cbor {
  CborEncodeStream &CborEncodeStream::operator<<(const std::string &str) {
    addCount(1);

    std::vector<uint8_t> encoded(9 + str.size());
    CborEncoder encoder;
    cbor_encoder_init(&encoder, encoded.data(), encoded.size(), 0);
    cbor_encode_text_string(&encoder, str.data(), str.size());
    data_.insert(data_.end(),
                 encoded.begin(),
                 encoded.begin()
                     + cbor_encoder_get_buffer_size(&encoder, encoded.data()));
    return *this;
  }

  CborEncodeStream &CborEncodeStream::operator<<(
      const libp2p::multi::ContentIdentifier &cid) {
    addCount(1);

    auto maybe_cid_bytes = libp2p::multi::ContentIdentifierCodec::encode(cid);
    if (maybe_cid_bytes.has_error()) {
      outcome::raise(CborEncodeError::INVALID_CID);
    }
    auto cid_bytes = maybe_cid_bytes.value();
    cid_bytes.insert(cid_bytes.begin(), 0);

    std::vector<uint8_t> encoded(18 + cid_bytes.size());
    CborEncoder encoder;
    cbor_encoder_init(&encoder, encoded.data(), encoded.size(), 0);
    cbor_encode_tag(&encoder, kCidTag);
    cbor_encode_byte_string(&encoder, cid_bytes.data(), cid_bytes.size());
    data_.insert(data_.end(),
                 encoded.begin(),
                 encoded.begin()
                     + cbor_encoder_get_buffer_size(&encoder, encoded.data()));
    return *this;
  }

  CborEncodeStream &CborEncodeStream::operator<<(
      const CborEncodeStream &other) {
    addCount(other.is_list_ ? 1 : other.count_);
    auto other_data = other.data();
    data_.insert(data_.end(), other_data.begin(), other_data.end());
    return *this;
  }

  struct LessCborKey {
    bool operator()(const std::vector<uint8_t> &lhs,
                    const std::vector<uint8_t> &rhs) const {
      ssize_t diff = lhs.size() - rhs.size();
      if (diff != 0) {
        return diff < 0;
      }
      return std::memcmp(lhs.data(), rhs.data(), lhs.size()) < 0;
    }
  };

  CborEncodeStream &CborEncodeStream::operator<<(
      const std::map<std::string, CborEncodeStream> &map) {
    addCount(1);

    std::array<uint8_t, 9> prefix{0};
    CborEncoder encoder;
    CborEncoder container;
    cbor_encoder_init(&encoder, prefix.data(), prefix.size(), 0);
    cbor_encoder_create_map(&encoder, &container, map.size());
    data_.insert(data_.end(),
                 prefix.begin(),
                 prefix.begin()
                     + cbor_encoder_get_buffer_size(&container, prefix.data()));

    std::map<std::vector<uint8_t>, std::vector<uint8_t>, LessCborKey> sorted;
    for (const auto &pair : map) {
      if (pair.second.count_ != 1) {
        outcome::raise(CborEncodeError::EXPECTED_MAP_VALUE_SINGLE);
      }
      sorted.insert(std::make_pair((CborEncodeStream() << pair.first).data(),
                                   pair.second.data()));
    }
    for (const auto &pair : sorted) {
      data_.insert(data_.end(), pair.first.begin(), pair.first.end());
      data_.insert(data_.end(), pair.second.begin(), pair.second.end());
    }

    return *this;
  }

  std::vector<uint8_t> CborEncodeStream::data() const {
    if (!is_list_) {
      return data_;
    }
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

  CborEncodeStream CborEncodeStream::list() {
    CborEncodeStream stream;
    stream.is_list_ = true;
    return stream;
  }

  std::map<std::string, CborEncodeStream> CborEncodeStream::map() {
    return {};
  }

  void CborEncodeStream::addCount(size_t count) {
    count_ += count;
  }
}  // namespace fc::codec::cbor
