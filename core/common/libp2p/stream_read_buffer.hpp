/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/bytes.hpp"
#include "common/libp2p/stream_proxy.hpp"
#include "common/outcome.hpp"

namespace libp2p::connection {
  using fc::Bytes;
  using fc::BytesOut;

  struct StreamReadBuffer : StreamProxy,
                            std::enable_shared_from_this<StreamReadBuffer> {
    std::shared_ptr<Bytes> buffer;
    size_t begin{};
    size_t end{};

    StreamReadBuffer(std::shared_ptr<Stream> stream, size_t capacity)
        : StreamProxy{std::move(stream)},
          buffer{std::make_shared<Bytes>(capacity)} {}

    size_t size() const {
      return end - begin;
    }

    void readFull(BytesOut out, size_t n, ReadCallbackFunc cb) {
      readSome(out,
               out.size(),
               [weak{weak_from_this()}, out, n, cb{std::move(cb)}](
                   outcome::result<size_t> _r) {
                 OUTCOME_CB(auto r, _r);
                 if (auto self{weak.lock()}) {
                   const auto _r{static_cast<ptrdiff_t>(r)};
                   assert(_r <= out.size());
                   if (_r == out.size()) {
                     return cb(n);
                   }
                   self->readFull(out.subspan(r), n, std::move(cb));
                 }
               });
    }

    void read(BytesOut out, size_t n, ReadCallbackFunc cb) override {
      assert(out.size() >= static_cast<ptrdiff_t>(n));
      readFull(out.first(n), n, std::move(cb));
    }

    void readSome(BytesOut out, size_t n, ReadCallbackFunc cb) override {
      assert(out.size() >= static_cast<ptrdiff_t>(n));
      if (n == 0) {
        return cb(n);
      }
      if (size() != 0) {
        n = std::min(n, size());
        std::copy_n(buffer->begin() + begin, n, out.begin());
        begin += n;
        return cb(n);
      }
      stream->readSome(
          *buffer,
          buffer->size(),
          [weak{weak_from_this()}, out, n, cb{std::move(cb)}, buffer{buffer}](
              outcome::result<size_t> _r) {
            OUTCOME_CB(auto r, _r);
            if (auto self{weak.lock()}) {
              self->begin = 0;
              self->end = r;
              self->readSome(out, n, std::move(cb));
            }
          });
    }
  };
}  // namespace libp2p::connection
