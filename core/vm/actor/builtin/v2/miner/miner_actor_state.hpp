/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/builtin/states/miner_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::miner {

  struct MinerActorState : states::MinerActorState {
    outcome::result<Buffer> toCbor() const override;

    outcome::result<types::miner::MinerInfo> getInfo(
        IpldPtr ipld) const override;
    outcome::result<void> setInfo(IpldPtr ipld,
                                  const types::miner::MinerInfo &info) override;

    outcome::result<types::miner::Deadlines> makeEmptyDeadlines(
        IpldPtr ipld, const CID &empty_amt_cid) override;
    outcome::result<types::miner::Deadline> getDeadline(
        IpldPtr ipld, const CID &cid) const override;
  };
  CBOR_TUPLE(MinerActorState,
             miner_info,
             precommit_deposit,
             locked_funds,
             vesting_funds,
             fee_debt,
             initial_pledge_requirement,
             precommitted_sectors,
             precommitted_setctors_expiry,
             allocated_sectors,
             sectors,
             proving_period_start,
             current_deadline,
             deadlines,
             early_terminations)

}  // namespace fc::vm::actor::builtin::v2::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v2::miner::MinerActorState> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v2::miner::MinerActorState &state,
                     const Visitor &visit) {
      visit(state.vesting_funds);
      visit(state.precommitted_sectors);
      visit(state.precommitted_setctors_expiry);
      visit(state.allocated_sectors);
      visit(state.sectors);
      visit(state.deadlines);
    }
  };
}  // namespace fc::cbor_blake
