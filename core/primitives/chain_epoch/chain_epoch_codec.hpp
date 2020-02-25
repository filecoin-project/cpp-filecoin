/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PRIMITIVES_CHAIN_EPOCH_CODEC_HPP
#define CPP_FILECOIN_PRIMITIVES_CHAIN_EPOCH_CODEC_HPP

#include <string>

#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::primitives::chain_epoch {
  std::string uvarintKey(uint64_t value);

  std::string encodeToByteString(const ChainEpoch &epoch);
}  // namespace fc::primitives::chain_epoch

#endif  // CPP_FILECOIN_PRIMITIVES_CHAIN_EPOCH_CODEC_HPP
