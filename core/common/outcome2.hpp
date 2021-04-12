/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/enum.hpp"
#include "common/outcome.hpp"

namespace fc {
  template <typename E>
  inline auto enumStr(const E &e) {
    return std::string{typeid(decltype(e)).name()} + ":"
           + std::to_string(common::to_int(e));
  }

  enum class OutcomeError { kDefault = 1 };

  template <typename T>
  struct _Outcome {
    using O = outcome::result<T>;
    using E = typename O::error_type;

    O o;

    _Outcome() : o{outcome::failure(OutcomeError::kDefault)} {}
    template <typename... Args>
    _Outcome(Args &&...args) : o{std::forward<Args>(args)...} {}

    // try_operation_has_value
    bool has_value() const {
      return o.has_value();
    }
    // try_operation_return_as
    const E &error() const {
      return o.error();
    }

    operator O &&() && {
      return std::move(o);
    }

    operator bool() const {
      return o.has_value();
    }
    const E &operator~() const {
      return o.error();
    }
  };

  template <typename T>
  struct Outcome : _Outcome<T> {
    using _O = _Outcome<T>;
    using _O::_O;

    Outcome(outcome::result<T> &&o) : _O{std::move(o)} {}

    // try_operation_extract_value
    const T &value() const & {
      return _O::o.value();
    }
    T &value() & {
      return _O::o.value();
    }
    T &&value() && {
      return std::move(_O::o.value());
    }

    const T &operator*() const & {
      return _O::o.value();
    }
    T &operator*() & {
      return _O::o.value();
    }
    T &&operator*() && {
      return std::move(_O::o.value());
    }
    const T *operator->() const {
      return &_O::o.value();
    }
    T *operator->() {
      return &_O::o.value();
    }
    template <typename... A>
    void emplace(A &&...a) {
      *this = T{std::forward<A>(a)...};
    }
  };

  template <>
  struct Outcome<void> : _Outcome<void> {
    using _O = _Outcome<void>;
    using _O::_O;

    void value() const {
      o.value();
    }

    void operator*() const {
      o.value();
    }
  };

  template <typename F>
  Outcome<std::invoke_result_t<F>> outcomeCatch(const F &f) {
    try {
      return f();
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }
  }
}  // namespace fc

OUTCOME_HPP_DECLARE_ERROR(fc, OutcomeError);
