/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/message_receiver.hpp"

namespace fc::data_transfer {

  outcome::result<void> MessageReceiver::registerVoucherType(
      const std::string &type, std::shared_ptr<RequestValidator> validator) {
    auto res = voucher_validators_.try_emplace(type, validator);
    if (!res.second)
      return MessageReceiverError::VOUCHER_VALIDATOR_ALREADY_REGISTERED;

    return fc::outcome::success();
  }

  outcome::result<void> MessageReceiver::validateVoucher(
      const PeerInfo &sender, const DataTransferRequest &request) const {
    auto validator = voucher_validators_.find(request.voucher_type);
    if (validator == voucher_validators_.end()) {
      return MessageReceiverError::VOUCHER_VALIDATOR_NOT_FOUND;
    }
    OUTCOME_TRY(base_cid, CID::fromString(request.base_cid));
    OUTCOME_TRY(selector, IPLDNodeImpl::createFromRawBytes(request.selector));
    if (request.is_pull) {
      OUTCOME_TRY(validator->second->ValidatePull(
          sender, request.voucher, base_cid, selector));
    } else {
      OUTCOME_TRY(validator->second->ValidatePush(
          sender, request.voucher, base_cid, selector));
    }

    return outcome::success();
  }

}  // namespace fc::data_transfer

OUTCOME_CPP_DEFINE_CATEGORY(fc::data_transfer, MessageReceiverError, e) {
  using fc::data_transfer::MessageReceiverError;

  switch (e) {
    case MessageReceiverError::VOUCHER_VALIDATOR_ALREADY_REGISTERED:
      return "MessageReceiverError: voucher validator is already "
             "registered";
    case MessageReceiverError::VOUCHER_VALIDATOR_NOT_FOUND:
      return "MessageReceiverError: voucher validator not found";
  }

  return "unknown error";
}
