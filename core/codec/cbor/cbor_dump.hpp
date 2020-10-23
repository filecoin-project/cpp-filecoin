/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gsl/span>

namespace fc {
  using BytesIn = gsl::span<const uint8_t>;
  class CID;
  std::string dumpBytes(BytesIn);
  std::string dumpCid(const CID &);
  std::string dumpCbor(BytesIn);
}  // namespace fc
