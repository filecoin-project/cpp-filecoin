/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "common/outcome.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/quantize.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::RleBitset;

  struct BitfieldQueue {
    adt::Array<RleBitset> queue;
    QuantSpec quant;

    outcome::result<void> addToQueue(ChainEpoch raw_epoch,
                                     const RleBitset &values);
    outcome::result<void> cut(const RleBitset &to_cut);
    outcome::result<void> addManyToQueueValues(
        const std::map<ChainEpoch, std::vector<uint64_t>> &values);
    outcome::result<std::tuple<RleBitset, bool>> popUntil(ChainEpoch until);
  };

}  // namespace fc::vm::actor::builtin::types::miner

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::types::miner::BitfieldQueue> {
    template <typename Visitor>
    static void call(vm::actor::builtin::types::miner::BitfieldQueue &p,
                     const Visitor &visit) {
      visit(p.queue);
    }
  };
}  // namespace fc
