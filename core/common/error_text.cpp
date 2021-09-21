/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/error_text.hpp"

#include <atomic>
#include <cassert>
#include <limits>
#include <unordered_map>

namespace fc::error_text {
  constexpr auto kName{"ErrorText"};
  constexpr auto kUintBits{std::numeric_limits<unsigned int>::digits};

  struct CategoryAligned : std::error_category {
    const char *name() const noexcept override {
      return kName;
    }
    std::string message(int value) const override {
      // NOLINTNEXTLINE
      return (const char *)((uintptr_t)(unsigned int)value << kUintBits);
    }
  };

  struct Category : std::error_category {
    uintptr_t base;
    explicit Category(uintptr_t base) noexcept : base{base} {}
    const char *name() const noexcept override {
      return kName;
    }
    std::string message(int value) const override {
      // NOLINTNEXTLINE
      return (const char *)(base + (uintptr_t)(unsigned int)value);
    }
  };

  const CategoryAligned category_aligned;
  const Category category_0{0};

  auto &_categories() {
    static std::unordered_map<uintptr_t, Category> categories;
    return categories;
  }

  std::error_code _make_error_code(const char *message) {
    static std::atomic_flag lock = ATOMIC_FLAG_INIT;
    assert(message != nullptr);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const uintptr_t ptr{reinterpret_cast<const uintptr_t>(message)};
    static_assert(sizeof(unsigned int) == sizeof(int));
    const unsigned int offset = ptr;
    if (offset == 0) {
      static_assert(sizeof(uintptr_t) <= 2 * sizeof(unsigned int));
      // NOLINTNEXTLINE
      return {(int)(unsigned int)(ptr >> kUintBits), category_aligned};
    }
    const int value = static_cast<int>(offset);
    if constexpr (sizeof(int) >= sizeof(uintptr_t)) {
      return {value, category_0};
    } else {
      const auto base{ptr - offset};
      while (lock.test_and_set(std::memory_order_acquire))
        ;
      auto &categories{_categories()};
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
