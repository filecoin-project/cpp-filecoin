/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace fc::codec::cbor {
  constexpr uint8_t kExtraUint8{24};
  constexpr uint8_t kExtraUint16{25};
  constexpr uint8_t kExtraUint32{26};
  constexpr uint8_t kExtraUint64{27};

  constexpr uint64_t kExtraFalse{20};
  constexpr uint64_t kExtraTrue{21};
  constexpr uint64_t kExtraNull{22};
  constexpr uint64_t kExtraCid{42};
}  // namespace fc::codec::cbor
