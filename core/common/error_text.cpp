/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/error_text.hpp"

#include <atomic>
#include <unordered_map>

namespace fc::error_text {
  struct Category : std::error_category {
    uintptr_t base;
    explicit Category(uintptr_t base) noexcept : base{base} {}
    const char *name() const noexcept override {
      return "ErrorText";
    }
    std::string message(int value) const override {
      return (const char *)(base + (uintptr_t)(unsigned int)value);
    }
  };
  const Category category_0{0};
  std::unordered_map<uintptr_t, Category> categories;
  std::atomic_flag lock = ATOMIC_FLAG_INIT;

  std::error_code _make_error_code(const char *message) {
    const auto ptr{(uintptr_t)message};
    static_assert(sizeof(unsigned int) == sizeof(int));
    const auto offset{(unsigned int)ptr};
    const auto value{(int)offset};
    if constexpr (sizeof(int) >= sizeof(uintptr_t)) {
      return {value, category_0};
    } else {
      const auto base{ptr - offset};
      while (lock.test_and_set(std::memory_order_acquire))
        ;
      auto it{categories.find(base)};
      if (it == categories.end()) {
        it = categories.emplace(base, base).first;
      }
      const auto &category{it->second};
      lock.clear(std::memory_order_release);
      return {value, category};
    }
  }
}  // namespace fc::error_text
