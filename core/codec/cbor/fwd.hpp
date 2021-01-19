/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"

namespace fc::codec::cbor {
  template <typename T>
  outcome::result<Buffer> encode(const T &arg);
  template <typename T>
  outcome::result<T> decode(BytesIn input);
}  // namespace fc::codec::cbor
