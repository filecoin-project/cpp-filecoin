/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ENCODE_STREAM_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ENCODE_STREAM_HPP

#include "codec/cbor/cbor_common.hpp"

#include <array>
#include <vector>

#include <cbor.h>

namespace fc::codec::cbor {
  class CborEncodeStream {
   public:
    static constexpr auto is_cbor_encoder_stream = true;

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    CborEncodeStream &operator<<(T num) {
      addCount(1);
      std::array<uint8_t, 9> buffer{0};
      CborEncoder encoder;
      cbor_encoder_init(&encoder, buffer.data(), buffer.size(), 0);
      if (std::is_same_v<T, bool>) {
        cbor_encode_boolean(&encoder, num);
      } else if (std::is_unsigned_v<T>) {
        cbor_encode_uint(&encoder, static_cast<uint64_t>(num));
      } else {
        cbor_encode_int(&encoder, static_cast<int64_t>(num));
      }
      data_.insert(data_.end(),
                   buffer.begin(),
                   buffer.begin()
                       + cbor_encoder_get_buffer_size(&encoder, buffer.data()));
      return *this;
    }

    CborEncodeStream &operator<<(const std::string &str);
    CborEncodeStream &operator<<(const libp2p::multi::ContentIdentifier &cid);
    CborEncodeStream &operator<<(const CborEncodeStream &other);
    CborEncodeStream &operator<<(
        const std::map<std::string, CborEncodeStream> &map);
    std::vector<uint8_t> data() const;
    static CborEncodeStream list();
    static std::map<std::string, CborEncodeStream> map();

   private:
    void addCount(size_t count);

    bool is_list_{false};
    std::vector<uint8_t> data_{};
    size_t count_{0};
  };
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ENCODE_STREAM_HPP
