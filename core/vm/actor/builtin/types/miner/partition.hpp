/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "common/outcome.hpp"
#include "vm/actor/builtin/types/miner/expiration.hpp"
#include "vm/actor/builtin/types/miner/types.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::RleBitset;

  struct Partition {
    RleBitset sectors;
    RleBitset unproven;
    RleBitset faults;
    RleBitset recoveries;
    RleBitset terminated;
    adt::Array<ExpirationSet> expirations_epochs;  // quanted
    adt::Array<RleBitset> early_terminated;
    PowerPair live_power;
    PowerPair unproven_power;
    PowerPair faulty_power;
    PowerPair recovering_power;

    RleBitset liveSectors() const;
    RleBitset activeSectors() const;
    PowerPair activePower() const;

    // TODO tagirov
  };

  CBOR_TUPLE(Partition,
             sectors,
             faults,
             recoveries,
             terminated,
             expirations_epochs,
             early_terminated,
             live_power,
             faulty_power,
             recovering_power)

}  // namespace fc::vm::actor::builtin::types::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::types::miner::Partition> {
    template <typename Visitor>
    static void call(vm::actor::builtin::types::miner::Partition &p,
                     const Visitor &visit) {
      visit(p.expirations_epochs);
      visit(p.early_terminated);
    }
  };
}  // namespace fc::cbor_blake
