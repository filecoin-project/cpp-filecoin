/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor_state.hpp"

#include "vm/actor/builtin/types/miner/v0/deadline.hpp" //TODO (m.tagirov) remove

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::sector::getRegisteredWindowPoStProof;
  using types::miner::kWPoStPeriodDeadlines;

  outcome::result<Universal<types::miner::MinerInfo>> MinerActorState::getInfo()
      const {
    OUTCOME_TRY(info, miner_info.get());
    OUTCOME_TRYA(info->window_post_proof_type,
                 getRegisteredWindowPoStProof(info->seal_proof_type));
    return std::move(info);
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
    OUTCOME_TRY(deadline0, getCbor<Deadline>(ipld, cid));
    return std::move(deadline0);
  }
}  // namespace fc::vm::actor::builtin::v0::miner
