/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/persistent_block/persistent_block.hpp"

namespace fc::primitives::blockchain::block {
  PersistentBlock::PersistentBlock(CID cid, gsl::span<const uint8_t> data)
      : cid_(std::move(cid)) {
    data_.put(data);
  }

  const CID &PersistentBlock::getCID() const {
    return cid_;
  }

  const common::Buffer  &PersistentBlock::getRawBytes() const {
    return data_;
  }

}  // namespace fc::primitives::blockchain::block
