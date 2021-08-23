/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/deadline.hpp"

#include "adt/cid_t.hpp"
#include "codec/cbor/streams_annotation.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::SectorNumber;

  /**
   * Deadlines contain Deadline objects, describing the sectors due at the
   * given deadline and their state (faulty, terminated, recovering, etc.).
   */
  struct Deadlines {
    std::vector<adt::CbCidT<Universal<Deadline>>> due;

    outcome::result<void> updateDeadline(uint64_t deadline_id,
                                         const Universal<Deadline> &deadline);

    outcome::result<std::tuple<uint64_t, uint64_t>> findSector(
        SectorNumber sector_num) const;
  };
  CBOR_TUPLE(Deadlines, due)

}  // namespace fc::vm::actor::builtin::types::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::types::miner::Deadlines> {
    template <typename Visitor>
    static void call(vm::actor::builtin::types::miner::Deadlines &d,
                     const Visitor &visit) {
      for (auto &deadline : d.due) {
        visit(deadline);
      }
    }
  };
}  // namespace fc::cbor_blake
