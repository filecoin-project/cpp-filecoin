/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ENCODE_STREAM_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ENCODE_STREAM_HPP

#include <array>
#include <cbor.h>
#include <vector>

namespace fc::codec::cbor {
  class CborEncodeStream {
   public:
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    CborEncodeStream &operator<<(T num) {
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
      data_.insert(data_.end(), buffer.begin(), buffer.begin() + cbor_encoder_get_buffer_size(&encoder, buffer.data()));
      ++count_;
      return *this;
    }

    std::vector<uint8_t> list() const;

    std::vector<uint8_t> single() const;

   private:
    std::vector<uint8_t> data_{};
    size_t count_{0};
  };
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_ENCODE_STREAM_HPP
