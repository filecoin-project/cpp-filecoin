/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/impl/provider_data_transfer_request_validator.hpp"

#include "markets/storage/storage_datatransfer_voucher.hpp"

namespace fc::markets::storage::provider {

  ProviderDataTransferRequestValidator::ProviderDataTransferRequestValidator(
      std::shared_ptr<ProviderStateStore> provider_state_store)
      : provider_state_store_{std::move(provider_state_store)} {}

  outcome::result<void> ProviderDataTransferRequestValidator::validatePush(
      const PeerInfo &sender,
      std::vector<uint8_t> encoded_voucher,
      CID base_cid,
      std::shared_ptr<Selector> selector) {
    OUTCOME_TRY(
        voucher,
        codec::cbor::decode<StorageDataTransferVoucher>(encoded_voucher));
    OUTCOME_TRY(deal, provider_state_store_->get(voucher.proposal_cid));

    if (deal.client != sender) {
      return ProviderRequestValidatorError::WRONG_PEER;
    }
    if (deal.ref.root != base_cid) {
      return ProviderRequestValidatorError::WRONG_PAYLOAD_CID;
    }
    if (deal.state != StorageDealStatus::STORAGE_DEAL_UNKNOWN
        && deal.state != StorageDealStatus::STORAGE_DEAL_VALIDATING) {
      return ProviderRequestValidatorError::INACCEPTABLE_DEAL_STATE;
    }

    return outcome::success();
  }

  outcome::result<void> ProviderDataTransferRequestValidator::validatePull(
      const PeerInfo &receiver,
      std::vector<uint8_t> encoded_voucher,
      CID base_cid,
      std::shared_ptr<Selector> selector) {
    return ProviderRequestValidatorError::ERROR_NO_PUSH_ACCEPTED;
  }
}  // namespace fc::markets::storage::provider

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::provider,
                            ProviderRequestValidatorError,
                            e) {
  using fc::markets::storage::provider::ProviderRequestValidatorError;

  switch (e) {
    case ProviderRequestValidatorError::ERROR_NO_PUSH_ACCEPTED:
      return "ProviderRequestValidatorError: provider doesn't accept pull "
             "requests";
    case ProviderRequestValidatorError::WRONG_PEER:
      return "ProviderRequestValidatorError: proposal has another peer";
    case ProviderRequestValidatorError::WRONG_PAYLOAD_CID:
      return "ProviderRequestValidatorError: proposal has another payload cid";
    case ProviderRequestValidatorError::INACCEPTABLE_DEAL_STATE:
      return "ProviderRequestValidatorError: inacceptable deal state";
    default:
      return "ProviderRequestValidatorError: unknown error";
  }
}
