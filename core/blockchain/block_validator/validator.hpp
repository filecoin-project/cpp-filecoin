/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "storage/buffer_map.hpp"
#include "vm/runtime/env_context.hpp"

namespace fc::blockchain::block_validator {
  using primitives::block::BlockHeader;
  using primitives::tipset::TipsetCPtr;
  using vm::interpreter::InterpreterCache;
  using vm::runtime::EnvironmentContext;
  using vm::state::StateTreeImpl;

  using MapPtr = std::shared_ptr<storage::PersistentBufferMap>;

  struct BlockValidator {
    MapPtr kv;
    IpldPtr ipld;
    TsLoadPtr ts_load;
    std::shared_ptr<InterpreterCache> interpreter_cache;
    SharedMutexPtr ts_branches_mutex;

    BlockValidator(MapPtr kv, const EnvironmentContext &envx);

    outcome::result<void> validate(const TsBranchPtr &branch,
                                   const BlockHeader &block);
    outcome::result<void> validateMessages(const BlockHeader &block,
                                           const TipsetCPtr &ts,
                                           StateTreeImpl &tree);
  };
}  // namespace fc::blockchain::block_validator
