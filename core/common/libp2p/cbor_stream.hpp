/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/connection/stream.hpp>

#include "codec/cbor/cbor_codec.hpp"
#include "common/libp2p/cbor_buffering.hpp"

namespace fc::common::libp2p {
  /// Reads and writes cbor objects
  class CborStream : public std::enable_shared_from_this<CborStream> {
   public:
    using Stream = ::libp2p::connection::Stream;
    using ReadCallback = void(outcome::result<gsl::span<const uint8_t>>);
    using ReadCallbackFunc = std::function<ReadCallback>;
    using WriteCallbackFunc = Stream::WriteCallbackFunc;

    /// Max number of bytes to read at a time
    static constexpr size_t kReserveBytes = 4 << 10;

    explicit CborStream(std::shared_ptr<Stream> stream);

    /// Get underlying stream
    std::shared_ptr<Stream> stream() const;

    /// Read bytes of cbor object
    void readRaw(ReadCallbackFunc cb);

    /// Read cbor object
    template <typename T>
    void read(std::function<void(outcome::result<T>)> cb) {
      readRaw([cb{std::move(cb)}](auto input) {
        if (input.has_error()) {
          return cb(input.error());
        }
        cb(codec::cbor::decode<T>(input.value()));
      });
    }

    /// Write bytes of cbor object
    void writeRaw(gsl::span<const uint8_t> input, WriteCallbackFunc cb);

    /// Write cbor object
    template <typename T>
    void write(const T &value, WriteCallbackFunc cb) {
      auto encoded = codec::cbor::encode(value);
      if (!encoded) {
        return cb(encoded.error());
      }
      encoded_ = std::move(encoded.value());
      writeRaw(encoded_, std::move(cb));
    }

    void close() {
      stream_->close([s{stream_}](auto) {});
    }

   private:
    void readMore(ReadCallbackFunc cb);
    void consume(gsl::span<uint8_t> input, ReadCallbackFunc cb);

    /**
     * Persistent buffer of data to be written to the libp2p stream to maintain
     * data validity until the callback is executed
     */
    Buffer encoded_;
    std::shared_ptr<Stream> stream_;
    CborBuffering buffering_;
    std::vector<uint8_t> buffer_;
    size_t size_{};
  };
}  // namespace fc::common::libp2p
