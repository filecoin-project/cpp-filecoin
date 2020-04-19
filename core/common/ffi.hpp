/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_FFI_HPP
#define CPP_FILECOIN_CORE_COMMON_FFI_HPP

#include <memory>

#include "common/blob.hpp"

namespace fc::common::ffi {
  template <typename T, typename D>
  auto wrap(T *ptr, D deleter) {
    return std::unique_ptr<T, D>(ptr, deleter);
  }

  template <size_t size>
  auto array(const uint8_t (&rhs)[size]) {
    Blob<size> lhs;
    std::copy(std::begin(rhs), std::end(rhs), std::begin(lhs));
    return lhs;
  }
}  // namespace fc::common::ffi

#endif  // CPP_FILECOIN_CORE_COMMON_FFI_HPP
