/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fwd.hpp"
#include "primitives/block/block.hpp"

namespace fc::blockchain::production {
  using primitives::block::BlockTemplate;
  using primitives::block::BlockWithMessages;
  using vm::interpreter::InterpreterCache;

  outcome::result<BlockWithMessages> generate(
      InterpreterCache &interpreter_cache,
      TsLoadPtr ts_load,
      std::shared_ptr<Ipld> ipld,
      BlockTemplate t);
}  // namespace fc::blockchain::production
