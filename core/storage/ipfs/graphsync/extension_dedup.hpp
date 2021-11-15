/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_codec.hpp"
#include "storage/ipfs/graphsync/extension.hpp"

namespace fc::storage::ipfs::graphsync::extension::dedup {
  constexpr std::string_view kName{"graphsync/dedup-by-key"};

  inline Extension make(const std::string_view &key) {
    return {std::string{kName}, codec::cbor::encode(key).value()};
  }
}  // namespace fc::storage::ipfs::graphsync::extension::dedup
