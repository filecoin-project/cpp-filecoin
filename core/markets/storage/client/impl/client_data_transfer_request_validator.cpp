/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/client/impl/client_data_transfer_request_validator.hpp"

#include "markets/storage/storage_datatransfer_voucher.hpp"

namespace fc::markets::storage::client {

  ClientDataTransferRequestValidator::ClientDataTransferRequestValidator(
      std::shared_ptr<ClientStateStore> client_state_store)
      : client_state_store_{std::move(client_state_store)} {}

  outcome::result<void> ClientDataTransferRequestValidator::validatePush(
      const PeerInfo &sender,
      std::vector<uint8_t> voucher,
      CID base_cid,
      std::shared_ptr<Selector> selector) {
    return ClientRequestValidatorError::kErrorNoPushAccepted;
  }

  outcome::result<void> ClientDataTransferRequestValidator::validatePull(
      const PeerInfo &receiver,
      std::vector<uint8_t> encoded_voucher,
      CID base_cid,
      std::shared_ptr<Selector> selector) {
    OUTCOME_TRY(
        voucher,
        codec::cbor::decode<StorageDataTransferVoucher>(encoded_voucher));
    OUTCOME_TRY(deal, client_state_store_->get(voucher.proposal_cid));

    if (deal.miner != receiver) {
      return ClientRequestValidatorError::kWrongPeer;
    }
    if (deal.data_ref.root != base_cid) {
      return ClientRequestValidatorError::kWrongPayloadCID;
    }
    if (deal.state != StorageDealStatus::STORAGE_DEAL_UNKNOWN
        && deal.state != StorageDealStatus::STORAGE_DEAL_VALIDATING) {
      return ClientRequestValidatorError::kInacceptableDealState;
    }

    return outcome::success();
  }
}  // namespace fc::markets::storage::client

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::client,
                            ClientRequestValidatorError,
                            e) {
  using fc::markets::storage::client::ClientRequestValidatorError;

  switch (e) {
    case ClientRequestValidatorError::kErrorNoPushAccepted:
      return "ClientRequestValidatorError: client doesn't accept push requests";
    case ClientRequestValidatorError::kWrongPeer:
      return "ClientRequestValidatorError: proposal has another peer";
    case ClientRequestValidatorError::kWrongPayloadCID:
      return "ClientRequestValidatorError: proposal has another payload cid";
    case ClientRequestValidatorError::kInacceptableDealState:
      return "ClientRequestValidatorError: inacceptable deal state";
    default:
      return "ClientRequestValidatorError: unknown error";
  }
}
