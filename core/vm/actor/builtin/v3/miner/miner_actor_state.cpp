/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/miner/miner_actor_state.hpp"

#include "vm/actor/builtin/types/miner/v3/deadline.hpp" //TODO (m.tagirov) remove

namespace fc::vm::actor::builtin::v3::miner {
  using types::miner::kWPoStPeriodDeadlines;

  outcome::result<Universal<types::miner::MinerInfo>> MinerActorState::getInfo()
      const {
    return miner_info.get();
  }

  outcome::result<types::miner::Deadlines> MinerActorState::makeEmptyDeadlines(
      IpldPtr ipld, const CID &empty_amt_cid) {
    Deadline deadline{Deadline::makeEmpty(ipld, empty_amt_cid)};
    OUTCOME_TRY(deadline_cid, setCbor(ipld, deadline));
    return types::miner::Deadlines{
        std::vector(kWPoStPeriodDeadlines, deadline_cid)};
  }

  outcome::result<types::miner::Deadline> MinerActorState::getDeadline(
      IpldPtr ipld, const CID &cid) const {
    OUTCOME_TRY(deadline3, getCbor<Deadline>(ipld, cid));
    return std::move(deadline3);
  }
}  // namespace fc::vm::actor::builtin::v3::miner
