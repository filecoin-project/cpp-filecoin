/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "deal_info_manager_impl.hpp"

#include "vm/actor/builtin/types/market/publish_deals_result.hpp"

namespace fc::vm::actor::builtin::types::market::deal_info_manager {
  using v0::market::PublishStorageDeals;

  outcome::result<CurrentDealInfo> DealInfoManagerImpl::getCurrentDealInfo(
      const Universal<DealProposal> &proposal, const CID &publish_cid) {
    // TODO (ortyomka): maybe async call, it's long
    OUTCOME_TRY(publish_message_wait,
                api_->StateSearchMsg(
                    TipsetKey{}, publish_cid, api::kLookbackNoLimit, true));
    if (!publish_message_wait.has_value()) {
      OUTCOME_TRY(cid_str, publish_cid.toString());
      logger_->error("looking for publish deal message {}: message not found",
                     cid_str);
      return DealInfoManagerError::kNotFound;
    }
    OUTCOME_TRY(
        deal_id,
        dealIdFromPublishDealsMsg(publish_message_wait.value(), proposal));

    OUTCOME_TRY(
        market_deal,
        api_->StateMarketStorageDeal(deal_id, publish_message_wait->tipset));

    OUTCOME_TRY(
        equal,
        checkProposalEquality(
            publish_message_wait->tipset, proposal, market_deal.proposal));
    if (not equal) {
      OUTCOME_TRY(cid_str, publish_cid.toString());
      logger_->error("deal proposals for publish message {} did not match",
                     cid_str);
      return DealInfoManagerError::kDealProposalNotMatch;
    }

    return CurrentDealInfo{
        .deal_id = deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = publish_message_wait->tipset,
    };
  }

  DealInfoManagerImpl::DealInfoManagerImpl(std::shared_ptr<FullNodeApi> api)
      : api_(std::move(api)),
        logger_(common::createLogger("deal info manager")) {}

  outcome::result<DealId> DealInfoManagerImpl::dealIdFromPublishDealsMsg(
      const MsgWait &publish_message_wait,
      const Universal<DealProposal> &proposal) {
    if (publish_message_wait.receipt.exit_code != VMExitCode::kOk) {
      OUTCOME_TRY(cid_str, publish_message_wait.message.toString());
      logger_->error(
          "looking for publish deal message {}: non-ok exit code: {}",
          cid_str,
          publish_message_wait.receipt.exit_code);
      return DealInfoManagerError::kNotOkExitCode;
    }

    OUTCOME_TRY(publish_message,
                api_->ChainGetMessage(publish_message_wait.message));

    OUTCOME_TRY(publish_deal_params,
                codec::cbor::decode<PublishStorageDeals::Params>(
                    publish_message.params));

    bool is_found = false;
    size_t deal_index = 0;
    for (size_t i = 0; i < publish_deal_params.deals.size(); i++) {
      OUTCOME_TRY(equal,
                  checkProposalEquality(publish_message_wait.tipset,
                                        proposal,
                                        publish_deal_params.deals[i].proposal));

      if (equal) {
        deal_index = i;
        is_found = true;
        break;
      }
    }

    if (not is_found) {
      OUTCOME_TRY(cid_str, publish_message_wait.message.toString());
      logger_->error("could not find deal in publish deals message {}",
                     cid_str);
      return DealInfoManagerError::kNotFound;
    }

    // get deal id from retval deal ids, deal proposal index == retval index
    OUTCOME_TRY(network_version,
                api_->StateNetworkVersion(publish_message_wait.tipset));
    return vm::actor::builtin::types::market::publishDealsResult(
        publish_message_wait.receipt.return_value,
        actorVersion(network_version),
        deal_index);
  }

  outcome::result<bool> DealInfoManagerImpl::checkProposalEquality(
      const TipsetKey &tipset_key,
      Universal<DealProposal> lhs,
      Universal<DealProposal> rhs) {
    OUTCOME_TRYA(lhs->client, api_->StateLookupID(lhs->client, tipset_key));
    OUTCOME_TRYA(rhs->client, api_->StateLookupID(rhs->client, tipset_key));

    return lhs == rhs;
  }
}  // namespace fc::vm::actor::builtin::types::market::deal_info_manager

OUTCOME_CPP_DEFINE_CATEGORY(
    fc::vm::actor::builtin::types::market::deal_info_manager,
    DealInfoManagerError,
    e) {
  using fc::vm::actor::builtin::types::market::deal_info_manager::
      DealInfoManagerError;
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
