/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/market/deal_info_manager/deal_info_manager.hpp"

#include "common/logger.hpp"

namespace fc::vm::actor::builtin::types::market::deal_info_manager {
  using api::FullNodeApi;
  using common::Logger;

  class DealInfoManagerImpl : public DealInfoManager {
   public:
    explicit DealInfoManagerImpl(std::shared_ptr<FullNodeApi> api);

    outcome::result<CurrentDealInfo> getCurrentDealInfo(
        const Universal<vm::actor::builtin::types::market::DealProposal> &proposal, const CID &publish_cid) override;

    outcome::result<DealId> dealIdFromPublishDealsMsg(
        const MsgWait &publish_message_wait,
        const Universal<vm::actor::builtin::types::market::DealProposal> &proposal) override;

   private:
    outcome::result<bool> checkProposalEquality(const TipsetKey &tipset_key,
                                                Universal<vm::actor::builtin::types::market::DealProposal> lhs,
                                                Universal<vm::actor::builtin::types::market::DealProposal> rhs);

    std::shared_ptr<FullNodeApi> api_;
    Logger logger_;
  };
}  // namespace fc::vm::actor::builtin::types::market::deal_info_manager
