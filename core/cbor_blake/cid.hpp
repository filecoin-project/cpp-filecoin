/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/bytes.hpp"

namespace fc {
  constexpr BytesN<6> kCborBlakePrefix{0x01, 0x71, 0xA0, 0xE4, 0x02, 0x20};

  using CbCid = common::Hash256;
}  // namespace fc
