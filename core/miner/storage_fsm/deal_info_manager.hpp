/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "primitives/types.hpp"

namespace fc::mining {
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
        const TipsetKey &tipset_key,
        const boost::optional<DealProposal> &proposal,
        const CID &publish_cid) = 0;
  };

  enum class DealInfoManagerError {
    kDealProposalNotMatch = 1,
    kOutOfRange,
    kNotFound,
    kMoreThanOneDeal,
    kNotOkExitCode,
  };
}  // namespace fc::mining

OUTCOME_HPP_DECLARE_ERROR(fc::mining, DealInfoManagerError);
