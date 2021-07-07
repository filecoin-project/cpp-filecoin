/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/miner/miner_actor_state.hpp"

#include "vm/actor/builtin/v2/miner/types/types.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::sector::getRegisteredWindowPoStProof;
  using types::miner::kWPoStPeriodDeadlines;

  ACTOR_STATE_TO_CBOR_THIS(MinerActorState)

  outcome::result<types::miner::MinerInfo> MinerActorState::getInfo(
      IpldPtr ipld) const {
    OUTCOME_TRY(info, getCbor<MinerInfo>(ipld, miner_info));
    OUTCOME_TRYA(info.window_post_proof_type,
                 getRegisteredWindowPoStProof(info.seal_proof_type));
    return std::move(info);
  }

  outcome::result<void> MinerActorState::setInfo(
      IpldPtr ipld, const types::miner::MinerInfo &info) {
    MinerInfo info2{info};
    OUTCOME_TRYA(miner_info, setCbor(ipld, info2));
    return outcome::success();
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
    OUTCOME_TRY(deadline2, getCbor<Deadline>(ipld, cid));
    return std::move(deadline2);
  }
}  // namespace fc::vm::actor::builtin::v2::miner
