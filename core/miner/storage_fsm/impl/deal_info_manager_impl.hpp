/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/deal_info_manager.hpp"

#include "api/full_node/node_api.hpp"
#include "common/logger.hpp"

namespace fc::mining {
  using api::FullNodeApi;
  using common::Logger;

  class DealInfoManagerImpl : public DealInfoManager {
   public:
    DealInfoManagerImpl(std::shared_ptr<FullNodeApi> api);

    outcome::result<CurrentDealInfo> getCurrentDealInfo(
        const TipsetKey &tipset_key,
        const boost::optional<DealProposal> &proposal,
        const CID &publish_cid) override;

   private:
    struct DealFromMessage {
      DealId deal_id{};
      TipsetKey publish_msg_tipset;
    };

    outcome::result<DealFromMessage> dealIdFromPublishDealsMsg(
        const TipsetKey &tipset_key,
        const boost::optional<DealProposal> &proposal,
        const CID &publish_cid);

    outcome::result<bool> checkDealEquality(const TipsetKey &tipset_key,
                                            DealProposal lhs,
                                            DealProposal rhs);

    std::shared_ptr<FullNodeApi> api_;
    Logger logger_;
  };
}  // namespace fc::mining
