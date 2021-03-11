/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/miner/miner_actor_state.hpp"

#include "vm/actor/builtin/v2/miner/types.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using types::miner::kWPoStPeriodDeadlines;

  outcome::result<Buffer> MinerActorState::toCbor() const {
    return Ipld::encode(*this);
  }

  outcome::result<types::miner::MinerInfo> MinerActorState::getInfo(
      IpldPtr ipld) const {
    OUTCOME_TRY(info2, ipld->getCbor<MinerInfo>(miner_info));
    return std::move(info2);
  }

  outcome::result<void> MinerActorState::setInfo(
      IpldPtr ipld, const types::miner::MinerInfo &info) {
    MinerInfo info2{info};
    OUTCOME_TRYA(miner_info, ipld->setCbor(info2));
    return outcome::success();
  }

  outcome::result<types::miner::Deadlines> MinerActorState::makeEmptyDeadlines(
      IpldPtr ipld, const CID &empty_amt_cid) {
    Deadline deadline{Deadline::makeEmpty(ipld, empty_amt_cid)};
    OUTCOME_TRY(deadline_cid, ipld->setCbor(deadline));
    return types::miner::Deadlines{
        std::vector(kWPoStPeriodDeadlines, deadline_cid)};
  }

  outcome::result<types::miner::Deadline> MinerActorState::getDeadline(
      IpldPtr ipld, const CID &cid) const {
    OUTCOME_TRY(deadline2, ipld->getCbor<Deadline>(cid));
    return std::move(deadline2);
  }
}  // namespace fc::vm::actor::builtin::v2::miner
