/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/chain_epoch/chain_epoch_codec.hpp"

#include <libp2p/multi/uvarint.hpp>

namespace fc::primitives::chain_epoch {

  using libp2p::multi::UVarint;

  std::string encodeToByteString(const ChainEpoch &epoch) {
    // TODO (a.chernyshov) actor-specs uses Protobuf Varint encoding
    UVarint number(epoch);
    auto encoded = number.toBytes();
    return std::string(encoded.begin(), encoded.end());
  }

}  // namespace fc::primitives::chain_epoch
