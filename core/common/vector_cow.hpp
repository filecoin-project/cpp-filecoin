/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gsl/span>
#include <variant>

namespace fc {

  template <typename T>
  class VectorCoW {
   public:
    using VectorType = std::vector<T>;
    using SpanType = gsl::span<const T>;

    VectorCoW() = default;
    // NOLINTNEXTLINE(google-explicit-constructor)
    VectorCoW(VectorType &&vector) : variant_{std::move(vector)} {}
    VectorCoW(const VectorType &vector) = delete;
    // NOLINTNEXTLINE(google-explicit-constructor)
    VectorCoW(const SpanType &span) : variant_{span} {}
    VectorCoW(const VectorCoW<T> &) = delete;
    VectorCoW(VectorCoW<T> &&) = default;
    VectorCoW<T> &operator=(const VectorCoW<T> &) = delete;
    VectorCoW<T> &operator=(VectorCoW<T> &&) = default;
    ~VectorCoW() = default;

    bool owned() const {
      return variant_.index() == 1;
    }

    SpanType span() const {
      if (!owned()) {
        return std::get<SpanType>(variant_);
      }
      return SpanType{std::get<VectorType>(variant_)};
    }

    size_t size() const {
      return span().size();
    }

    bool empty() const {
      return span().empty();
    }

    // get mutable container reference, copy once if span
    VectorType &mut() {
      if (!owned()) {
        const auto span = std::get<SpanType>(variant_);
        variant_.template emplace<VectorType>(span.begin(), span.end());
      }
      return std::get<VectorType>(variant_);
    }

    // move container away, copy once if span
    VectorType into() {
      auto vector{std::move(mut())};
      variant_.template emplace<SpanType>();
      return vector;
    }

   private:
    std::variant<SpanType, VectorType> variant_;
  };
}  // namespace fc
