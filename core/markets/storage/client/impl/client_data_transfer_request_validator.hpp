/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_CLIENT_CLIENT_DATA_TRANSFER_REQUEST_VALIDATOR_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_CLIENT_CLIENT_DATA_TRANSFER_REQUEST_VALIDATOR_HPP

#include <libp2p/peer/peer_info.hpp>
#include "data_transfer/request_validator.hpp"
#include "fsm/state_store.hpp"
#include "markets/storage/client/impl/client_state_store.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "storage/ipld/selector.hpp"

namespace fc::markets::storage::client {
  using data_transfer::RequestValidator;
  using fc::storage::ipld::Selector;
  using libp2p::peer::PeerInfo;

  class ClientDataTransferRequestValidator : public RequestValidator {
   public:
    ClientDataTransferRequestValidator(
        std::shared_ptr<ClientStateStore> client_state_store);

    /**
     * Will always error because clients should not accept push requests from a
     * provider in a storage deal (i.e. send data to client)
     * @return Error
     */
    outcome::result<void> validatePush(
        const PeerInfo &sender,
        std::vector<uint8_t> voucher,
        CID base_cid,
        std::shared_ptr<Selector> selector) override;

    /**
     * Will succeed only if:
     * - voucher has correct type
     * - voucher references an active deal
     * - referenced deal matches the receiver (miner)
     * - referenced deal matches the given base CID
     * - referenced deal is in an acceptable state
     * @return success if pull is accepted, false otherwise
     */
    outcome::result<void> validatePull(
        const PeerInfo &receiver,
        std::vector<uint8_t> voucher,
        CID base_cid,
        std::shared_ptr<Selector> selector) override;

   private:
    std::shared_ptr<ClientStateStore> client_state_store_;
  };

  enum class ClientRequestValidatorError {
    kErrorNoPushAccepted,
    kWrongPeer,
    kWrongPayloadCID,
    kInacceptableDealState,
  };

}  // namespace fc::markets::storage::client

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::client,
                          ClientRequestValidatorError);

#endif  // CPP_FILECOIN_MARKETS_STORAGE_CLIENT_CLIENT_DATA_TRANSFER_REQUEST_VALIDATOR_HPP
