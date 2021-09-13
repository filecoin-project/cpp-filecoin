/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/deal_info_manager_impl.hpp"

#include "vm/actor/builtin/v0/market/market_actor.hpp"

namespace fc::mining {
  using vm::VMExitCode;
  using vm::actor::builtin::v0::market::PublishStorageDeals;

  outcome::result<CurrentDealInfo> DealInfoManagerImpl::getCurrentDealInfo(
      const TipsetKey &tipset_key,
      const boost::optional<DealProposal> &proposal,
      const CID &publish_cid) {
    OUTCOME_TRY(deal,
                dealIdFromPublishDealsMsg(tipset_key, proposal, publish_cid));

    OUTCOME_TRY(market_deal,
                api_->StateMarketStorageDeal(deal.deal_id, tipset_key));

    if (proposal.has_value()) {
      OUTCOME_TRY(equal,
                  checkDealEquality(
                      tipset_key, proposal.value(), market_deal.proposal));
      if (not equal) {
        OUTCOME_TRY(cid_str, publish_cid.toString());
        logger_->error("deal proposals for publish message {} did not match",
                       cid_str);
        return DealInfoManagerError::kDealProposalNotMatch;
      }
    }

    return CurrentDealInfo{
        .deal_id = deal.deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = deal.publish_msg_tipset,
    };
  }

  DealInfoManagerImpl::DealInfoManagerImpl(std::shared_ptr<FullNodeApi> api)
      : api_(std::move(api)),
        logger_(common::createLogger("deal info manager")) {}

  outcome::result<DealInfoManagerImpl::DealFromMessage>
  DealInfoManagerImpl::dealIdFromPublishDealsMsg(
      const TipsetKey &tipset_key,
      const boost::optional<DealProposal> &proposal,
      const CID &publish_cid) {
    // TODO: maybe async call, it's long
    OUTCOME_TRY(lookup,
                api_->StateSearchMsg(
                    TipsetKey{}, publish_cid, api::kLookbackNoLimit, true));
    if (lookup->receipt.exit_code != VMExitCode::kOk) {
      OUTCOME_TRY(cid_str, publish_cid.toString());
      logger_->error(
          "looking for publish deal message {}: non-ok exit code: {}",
          cid_str,
          lookup->receipt.exit_code);
      return DealInfoManagerError::kNotOkExitCode;
    }

    OUTCOME_TRY(return_value,
                codec::cbor::decode<PublishStorageDeals::Result>(
                    lookup->receipt.return_value));

    if (not proposal.has_value()) {
      if (return_value.deals.size() > 1) {
        OUTCOME_TRY(cid_str, publish_cid.toString());
        logger_->error(
            "getting deal ID from publish deal message {}: no deal proposal "
            "supplied but message return value has more than one deal ({} "
            "deals)",
            cid_str,
            return_value.deals.size());
        return DealInfoManagerError::kMoreThanOneDeal;
      }

      return DealFromMessage{
          .deal_id = return_value.deals[0],
          .publish_msg_tipset = lookup->tipset,
      };
    }

    OUTCOME_TRY(public_message, api_->ChainGetMessage(publish_cid));
    OUTCOME_TRY(public_deals_params,
                codec::cbor::decode<PublishStorageDeals::Params>(
                    public_message.params));

    bool is_found = false;
    DealId deal_id = 0;
    for (size_t i = 0; i < public_deals_params.deals.size(); i++) {
      OUTCOME_TRY(equal,
                  checkDealEquality(tipset_key,
                                    proposal.value(),
                                    public_deals_params.deals[i].proposal));

      if (equal) {
        deal_id = i;
        is_found = true;
        break;
      }
    }

    if (not is_found) {
      OUTCOME_TRY(cid_str, publish_cid.toString());
      logger_->error("could not find deal in publish deals message {}",
                     cid_str);
      return DealInfoManagerError::kNotFound;
    }

    if (deal_id >= return_value.deals.size()) {
      OUTCOME_TRY(cid_str, publish_cid.toString());
      logger_->error(
          "deal index {} out of bounds of deals (size {}) in publish deals "
          "message {}",
          deal_id,
          return_value.deals.size(),
          cid_str);
      return DealInfoManagerError::kOutOfRange;
    }

    return DealFromMessage{
        .deal_id = return_value.deals[deal_id],
        .publish_msg_tipset = lookup->tipset,
    };
  }

  outcome::result<bool> DealInfoManagerImpl::checkDealEquality(
      const TipsetKey &tipset_key, DealProposal lhs, DealProposal rhs) {
    OUTCOME_TRYA(lhs.client, api_->StateLookupID(lhs.client, tipset_key));
    OUTCOME_TRYA(rhs.client, api_->StateLookupID(rhs.client, tipset_key));

    return lhs == rhs;
  }
}  // namespace fc::mining

OUTCOME_CPP_DEFINE_CATEGORY(fc::mining, DealInfoManagerError, e) {
  using fc::mining::DealInfoManagerError;
  switch (e) {
    case DealInfoManagerError::kDealProposalNotMatch:
      return "Deal info manager: deal proposals for publish message did not "
             "match";
    case DealInfoManagerError::kOutOfRange:
      return "Deal info manager: deal index out of bounds of deals in publish "
             "deals message";
    case DealInfoManagerError::kNotFound:
      return "Deal info manager: could not find deal in publish deals message";
    case DealInfoManagerError::kMoreThanOneDeal:
      return "Deal info manager: no deal proposal supplied but message return "
             "value has more than one deal";
    case DealInfoManagerError::kNotOkExitCode:
      return "Deal info manager: looking for publish deal message: non-ok exit "
             "code";
  }
  return "Deal info manager: unknown error";
}
