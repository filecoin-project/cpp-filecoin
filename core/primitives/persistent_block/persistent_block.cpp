/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/persistent_block/persistent_block.hpp"

namespace fc::primitives::blockchain::block {
  const CID &PersistentBlock::getCID() const {
    return cid_;
  }

  const PersistentBlock::Content &PersistentBlock::getRawBytes() const {
    return data_;
  }

}  // namespace fc::blockchain::block
