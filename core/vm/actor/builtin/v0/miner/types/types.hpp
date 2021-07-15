/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/deadline.hpp"

namespace fc::vm::actor::builtin::v0::miner {

  struct Deadline : types::miner::Deadline {
    Deadline() = default;
    Deadline(const types::miner::Deadline &other)
        : types::miner::Deadline(other) {}
    Deadline(types::miner::Deadline &&other) : types::miner::Deadline(other) {}
  };
  CBOR_TUPLE(Deadline,
             partitions,
             expirations_epochs,
             post_submissions,
             early_terminations,
             live_sectors,
             total_sectors,
             faulty_power)

}  // namespace fc::vm::actor::builtin::v0::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v0::miner::Deadline> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::miner::Deadline &d,
                     const Visitor &visit) {
      visit(d.partitions);
      visit(d.expirations_epochs);
    }
  };
}  // namespace fc::cbor_blake
