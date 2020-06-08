/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_TYPES_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_TYPES_HPP

#include <string>

#include "markets/retrieval/types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"

namespace fc::markets::retrieval::client {
  using primitives::TokenAmount;
  using primitives::address::Address;

  /**
   * @struct Client-side parameters for a new deal
   */
  struct DealProfile {
    /* Proposed price */
    TokenAmount price_per_byte;

    /* Number of bytes before the next payment */
    uint64_t payment_interval;

    /* Rate at which payment interval value increases */
    uint64_t payment_interval_increase;

    /* Max tokens amount, which can be spent */
    TokenAmount total_funds;

    /* Client's wallet */
    Address client_address;

    /* Miner's wallet */
    Address payment_address;
  };

  /*
   * @enum Client-side deal events
   */
  enum class DealStatus : uint16_t {
    /* Deal was initiated */
    DealOpen = 1,

    /* Creating payment channel error */
    PaymentChannelError,

    /* Creating lane in the payment channel error */
    AllocateLaneError,

    /* Waiting for a message to create a payment channel */
    PaymentChannelCreateInitiated,

    /* Newly created payment channel is ready to for the deal to resume */
    PaymentChannelReady,

    /* Waiting for funds to be added to payment channel */
    PaymentChannelAddingFunds,

    /* Failed to add funds to payment channel */
    PaymentChannelAddFundsError,

    /* Send deal proposal network error */
    WriteDealProposalError,

    /* Receive deal proposal response network error */
    ReadDealResponseError,

    /* Provider rejected deal */
    DealRejected,

    /* Provider couldn't find a piece for a deal */
    DealNotFound,

    /* Provider accepted a deal */
    DealAccepted,

    /* Received unknown deal proposal response from provider */
    UnknownResponseReceived,

    /* Need to add more funds to a payment channel to continue a deal */
    FundsExpended,

    /* Provider asked for funds in a way that doesn't match the deal terms */
    BadPaymentRequested,

    /* Failed to create payment voucher */
    CreateVoucherFailed,

    /* Send payment voucher network error */
    WriteDealPaymentError,

    /* Payment voucher was sent to a provider */
    PaymentSent,

    /* Network error while reading block from provider */
    ConsumeBlockError,

    /* Provider requested last payment */
    LastPaymentRequested,

    /* Provider has sent all blocks */
    AllBlocksReceived,

    /* Provider completed a deal without sending all blocks */
    EarlyTermination,

    /* Provider requested a next payment */
    PaymentRequested,

    /* Received next data from a provider */
    DealProgress,

    /* Error occurred during a deal */
    DealFailed,

    /* Deal has been completed */
    DealComplete
  };

  /**
   * @struct Client-side states of a deal
   */
  struct ClientDealState { /* Total funds, which can be spent */
    TokenAmount total_tokens;

    /* Client address */
    Address client_wallet;

    /* Miner address */
    Address miner_wallet;

    /* Payment channel and lane id */
    PaymentInfo payment_info;

    /* Current deal status */
    DealStatus deal_status;

    /* Sender peer id */
    uint64_t sender_peer_id;

    /* Already received bytes count */
    uint64_t total_received;

    /* Optional message */
    std::string message;

    /* Already paid bytes */
    uint64_t bytes_paid_for;

    /* Current payment interval, bytes */
    uint64_t current_interval;

    /* Requested payment valued */
    TokenAmount payment_requested;

    /* Already spent value */
    TokenAmount tokens_spent;

    /* Signal message id */
    CID wait_message_cid;
  };

  /*
   * @enum Client deal FSM states
   */
  enum class ClientState {
    /* Ready to send a deal proposal to a provider */
    DealNew,

    /* Deal proposal to a provider was sent */
    DealOpen,

    /* Deal proposal was rejected by provider,
     * or deal was early terminated by provider */
    DealRejected,

    /* Receiving blocks of the requested Piece in progress */
    DealOngoing,

    /* Occurred error during retrieval deal */
    DealFailed,

    /* Deal was successfully completed */
    DealFinished,

    /* Waiting payment channel creation result */
    CreatingPaymentChannel,

    /* Wating lane allocation result */
    AllocatingLane,

    /* Waiting adding funds result */
    AddingFunds,

    /* Waiting create voucher result */
    CreatingVoucher
  };

  /*
   * @enum Client-side deal lifecycle events
   */
  enum class ClientEvent : uint16_t {
    /* Send deal proposal to a provider */
    EvSendProposal = 1,

    /* Provider rejected a deal */
    EvDealRejected,

    /* Provider accepted a deal */
    EvDealAccepted,

    /* Receiving blocks from a provider progress */
    EvReceiveProgress,

    /* Network error while attempting to execute any action */
    EvNetworkError,

    /* Provider completed the deal without sending all blocks */
    EvEarlyTermination,

    /* Error during receiving block from a provider */
    EvBlockConsumeFail,

    /* Failed to verify received block */
    EvBlockVerifyFail,

    /* Failed to save received block */
    EvBlockWriteFail,

    /* Received unknown resoponse from a provider */
    EvUnknownResponse,

    /* Provider asked to send next payment */
    EvSendPayment,

    /* Provider asked for funds in a way, which violating terms of the deal */
    EvBadPaymentRequest,

    /* Failed to create payment channel */
    EvCreatePaymentError,

    /* Allocate lane in a payment channel */
    EvAllocateLane,

    /* Failed to allocate lane in a payment channel */
    EvAllocateLaneError,

    /* Add funds to a payment channel */
    EvAddFunds,

    /* Add funds to a payment channel error */
    EvAddFundsError,

    /* Create payment voucher */
    EvCreateVoucher,

    /* Create payment voucher error */
    EvCreateVoucherError,

    /* Error sending payment voucher to a provider */
    EvWritePaymentError,

    /* Payment voucher to a provider was successfully sent */
    EvPaymentSent,

    /* Retrieval deal successfully completed */
    EvDealCompleted
  };
}  // namespace fc::markets::retrieval::client

#endif
