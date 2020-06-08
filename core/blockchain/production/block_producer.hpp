/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/block/block.hpp"

namespace fc::blockchain::production {
  using primitives::block::Block;
  using primitives::block::BlockTemplate;

  constexpr size_t kBlockMaxMessagesCount = 1000;

  outcome::result<Block> generate(std::shared_ptr<Ipld> ipld, BlockTemplate t);
}  // namespace fc::blockchain::production
