/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cbor_blake/cid_block.hpp"
#include "codec/cbor/light_reader/cid.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::codec::cbor::light_reader {
  using primitives::ChainEpoch;

  inline bool readBlockParents(BlockParentCbCids &parents,
                               CborToken token,
                               BytesIn &input) {
    parents.resize(0);
    parents.mainnet_genesis = false;
    if (!token.listCount()) {
      return false;
    }
    const auto n{*token.listCount()};
    parents.reserve(n);
    for (auto i{n}; i; --i) {
      const CbCid *parent;
      if (!read(token, input).cidSize()) {
        return false;
      }
      if (n == 1 && startsWith(input, kMainnetGenesisBlockParent)) {
        parents.mainnet_genesis = true;
        if (!codec::read(input, *token.cidSize())) {
          return false;
        }
        break;
      }
      if (!readCborBlake(parent, token, input)) {
        return false;
      }
      parents.push_back(*parent);
    }
    return true;
  }

  inline bool readBlock(BytesIn &ticket,
                        BlockParentCbCids &parents,
                        ChainEpoch &height,
                        BytesIn &input) {
    CborToken token;
    if (read(token, input).listCount() != 16) {
      return false;
    }
    if (!skipNested(input, 1)) {
      return false;
    }
    if (read(token, input).listCount() != 1) {
      return false;
    }
    if (!read(token, input).bytesSize()
        || !codec::read(ticket, input, *token.bytesSize())) {
      return false;
    }
    if (!skipNested(input, 3)) {
      return false;
    }
    if (!read(token, input) || !readBlockParents(parents, token, input)) {
      return false;
    }
    if (!skipNested(input, 1)) {
      return false;
    }
    if (!read(token, input).asUint()) {
      return false;
    }
    height = *token.asUint();
    return true;
  }
}  // namespace fc::codec::cbor::light_reader
