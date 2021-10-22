/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <variant>

#include "common/bytes.hpp"

namespace fc {
  struct BytesCow {
    // span or vector
    // note: variant can be implemented as simple c++ union
    std::variant<BytesIn, Bytes> variant;

    BytesCow() = default;
    // move vector into
    BytesCow(Bytes &&vector) : variant{std::move(vector)} {}
    BytesCow(const Bytes &) = delete;
    BytesCow(BytesIn span) : variant{span} {}
    BytesCow(const BytesCow &) = delete;
    BytesCow(BytesCow &&) = default;
    BytesCow &operator=(const BytesCow &) = delete;
    BytesCow &operator=(BytesCow &&) = default;

    // get span
    BytesIn ref() const {
      if (auto span{std::get_if<BytesIn>(&variant)}) {
        return *span;
      }
      return BytesIn{std::get<Bytes>(variant)};
    }

    operator BytesIn() const {
      return ref();
    }

    size_t size() const {
      return ref().size();
    }

    // get mutable vector reference, copy once if span
    Bytes &mut() {
      if (auto span{std::get_if<BytesIn>(&variant)}) {
        variant.emplace<Bytes>(copy(*span));
      }
      return std::get<Bytes>(variant);
    }

    // move vector away, copy once if span
    Bytes into() {
      auto vector{std::move(mut())};
      variant.emplace<BytesIn>();
      return vector;
    }
  };

  // note: may rewrite code to reuse best of two vectors
  inline void copy(Bytes &l, BytesCow &&r) {
    copy(l, BytesIn{r});
  }
}  // namespace fc
