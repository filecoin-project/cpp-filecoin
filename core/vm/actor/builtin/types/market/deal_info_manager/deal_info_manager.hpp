/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "common/outcome.hpp"
#include "markets/storage/mk_protocol.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::market::deal_info_manager {
  using api::MsgWait;
  using markets::storage::DealProposal;
  using markets::storage::StorageDeal;
  using primitives::DealId;
  using primitives::tipset::TipsetKey;

  struct CurrentDealInfo {
    DealId deal_id;
    StorageDeal market_deal;
    TipsetKey publish_msg_tipset;
  };

  inline bool operator==(const CurrentDealInfo &lhs,
                         const CurrentDealInfo &rhs) {
    return lhs.deal_id == rhs.deal_id && lhs.market_deal == rhs.market_deal
           && lhs.publish_msg_tipset == rhs.publish_msg_tipset;
  };

  class DealInfoManager {
   public:
    virtual ~DealInfoManager() = default;

    virtual outcome::result<CurrentDealInfo> getCurrentDealInfo(
        const Universal<vm::actor::builtin::types::market::DealProposal> &proposal, const CID &publish_cid) = 0;

    /**
     * Returns published deal id.
     * Deal id is in PublishStorageDeals result call, it depends on client
     * proposal id.
     * @param publish_message_wait - state of the message with
     * PublishStorageDeals
     * @param client_deal_proposal - deal proposal in message
     */
    virtual outcome::result<DealId> dealIdFromPublishDealsMsg(
        const MsgWait &publish_message_wait,
        const Universal<vm::actor::builtin::types::market::DealProposal> &proposal) = 0;
  };

  enum class DealInfoManagerError {
    kDealProposalNotMatch = 1,
    kOutOfRange,
    kNotFound,
    kMoreThanOneDeal,
    kNotOkExitCode,
  };
}  // namespace fc::vm::actor::builtin::types::market::deal_info_manager

OUTCOME_HPP_DECLARE_ERROR(
    fc::vm::actor::builtin::types::market::deal_info_manager,
    DealInfoManagerError);
