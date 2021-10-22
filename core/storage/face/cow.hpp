/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/bytes_cow.hpp"

namespace fc::storage::face {
  template <typename T>
  struct cow;

  template <typename T>
  using cow_t = typename cow<T>::type;

  template <>
  struct cow<Bytes> {
    using type = BytesCow;
  };
}  // namespace fc::storage::face