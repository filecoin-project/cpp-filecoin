/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cpp-ledger/common/types.hpp"

#include <tuple>

namespace ledger::filecoin {
  using Chunks = std::vector<Bytes>;

  std::tuple<Bytes, Error> getBip44bytes(const std::vector<uint32_t> &bip44path,
                                         int hardenCount);

  std::tuple<Chunks, Error> prepareChunks(
      const std::vector<uint32_t> &bip44path, const Bytes &transaction);

}  // namespace ledger::filecoin
