/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <variant>

#include "common/bytes.hpp"

namespace fc {
  class BytesCow {
   private:
    std::variant<BytesIn, Bytes> variant;

   public:
    BytesCow() = default;
    // NOLINTNEXTLINE(google-explicit-constructor)
    BytesCow(Bytes &&vector) : variant{std::move(vector)} {}
    BytesCow(const Bytes &) = delete;
    // NOLINTNEXTLINE(google-explicit-constructor)
    BytesCow(const BytesIn &span) : variant{span} {}
    BytesCow(const BytesCow &) = delete;
    BytesCow(BytesCow &&) = default;
    BytesCow &operator=(const BytesCow &) = delete;
    BytesCow &operator=(BytesCow &&) = default;
    ~BytesCow() = default;

    bool owned() const {
      return variant.index() == 1;
    }

    BytesIn span() const {
      if (!owned()) {
        return std::get<BytesIn>(variant);
      }
      return BytesIn{std::get<Bytes>(variant)};
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator BytesIn() const {
      return span();
    }

    size_t size() const {
      return span().size();
    }

    // get mutable vector reference, copy once if span
    Bytes &mut() {
      if (!owned()) {
        variant.emplace<Bytes>(copy(std::get<BytesIn>(variant)));
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

  inline void copy(Bytes &l, BytesCow &&r) {
    copy(l, BytesIn{r});
  }
}  // namespace fc
