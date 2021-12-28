/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpp-ledger/filecoin/impl/utils.hpp"

namespace ledger::filecoin {

  std::tuple<Bytes, Error> getBip44bytes(const std::vector<uint32_t> &bip44path,
                                         int hardenCount) {
    // todo
    return std::make_tuple(Bytes{}, Error{});
  }

  std::tuple<Chunks, Error> prepareChunks(
      const std::vector<uint32_t> &bip44path, const Bytes &transaction) {
    // todo
    return std::make_tuple(Chunks{}, Error{});
  }

}  // namespace ledger::filecoin
