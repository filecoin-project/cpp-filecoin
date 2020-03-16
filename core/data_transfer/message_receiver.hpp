/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_MESSAGE_RECEIVER_HPP
#define CPP_FILECOIN_DATA_TRANSFER_MESSAGE_RECEIVER_HPP

#include <libp2p/peer/peer_info.hpp>
#include "common/logger.hpp"
#include "data_transfer/message.hpp"
#include "data_transfer/request_validator.hpp"
#include "storage/ipld/impl/ipld_node_impl.hpp"

namespace fc::data_transfer {

  using libp2p::peer::PeerInfo;
  using storage::ipld::IPLDNodeImpl;

  /**
   * MessageReceiver is an interface for receiving messages from the
   * GraphSyncNetwork.
   */
  class MessageReceiver {
   public:
    virtual ~MessageReceiver() = default;

    virtual outcome::result<void> receiveRequest(
        const PeerInfo &initiator, const DataTransferRequest &request) = 0;

    virtual void receiveResponse(const PeerInfo &sender,
                                 const DataTransferResponse &response) = 0;

    virtual void receiveError() = 0;

    /**
     * Register validator for voucher type
     * @param type - voucher type
     * @param validator for voucher
     * @return error in case of failure
     */
    outcome::result<void> registerVoucherType(
        const std::string &type, std::shared_ptr<RequestValidator> validator);

   protected:
    /**
     * validateVoucher converts a voucher in an incoming message to its
     * appropriate voucher struct, then runs the validator and returns the
     * results.
     * @param sender
     * @param incoming_request
     * @return error if:
     *  - voucherFromRequest fails
     *  - deserialization of selector fails
     *  - validation fails
     */
    outcome::result<void> validateVoucher(
        const PeerInfo &sender, const DataTransferRequest &request) const;

    common::Logger logger_ = common::createLogger("data_transfer");

   private:
    std::map<std::string, std::shared_ptr<RequestValidator>>
        voucher_validators_;
  };

  /**
   * @brief Type of errors returned by Keystore
   */
  enum class MessageReceiverError {
    VOUCHER_VALIDATOR_ALREADY_REGISTERED,
    VOUCHER_VALIDATOR_NOT_FOUND
  };

}  // namespace fc::data_transfer

OUTCOME_HPP_DECLARE_ERROR(fc::data_transfer, MessageReceiverError);

#endif  // CPP_FILECOIN_DATA_TRANSFER_MESSAGE_RECEIVER_HPP
