/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/bytes.hpp"
#include "common/outcome.hpp"

namespace fc::codec::cbor {
  template <typename T>
  // NOLINTNEXTLINE(readability-redundant-declaration)
  outcome::result<Bytes> encode(const T &arg);

  template <typename T>
  // NOLINTNEXTLINE(readability-redundant-declaration)
  outcome::result<T> decode(BytesIn input);
}  // namespace fc::codec::cbor
