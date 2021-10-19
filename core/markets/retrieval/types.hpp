/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/host/basic_host/basic_host.hpp>
#include <libp2p/peer/peer_id.hpp>
#include "codec/cbor/streams_annotation.hpp"

#include "common/libp2p/peer/cbor_peer_id.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/builtin/types/payment_channel/voucher.hpp"

namespace fc::markets::retrieval {
  using libp2p::peer::PeerId;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using vm::actor::builtin::types::payment_channel::LaneId;

  struct RetrievalAsk {
    TokenAmount price_per_byte;
    TokenAmount unseal_price;
    uint64_t payment_interval;
    uint64_t interval_increase;
  };
  CBOR_TUPLE(RetrievalAsk,
             price_per_byte,
             unseal_price,
             payment_interval,
             interval_increase)

  /**
   * @struct Payment info
   */
  struct PaymentInfo {
    Address payment_channel;
    LaneId lane{};
  };

  /**
   * Miner address and PeerId for retrieval deal
   */
  struct RetrievalPeer {
    Address address;
    PeerId peer_id{codec::cbor::kDefaultT<PeerId>()};
    boost::optional<CID> piece;

    inline bool operator==(const RetrievalPeer &other) const {
      return address == other.address && peer_id == other.peer_id
             && piece == other.piece;
    }
  };
  CBOR_TUPLE(RetrievalPeer, address, peer_id, piece)

  /**
   * @enum Deal statuses
   */
  enum class DealStatus : uint64_t {
    /* New deal, nothing happened with it */
    kDealStatusNew,

    kDealStatusUnsealing,
    kDealStatusUnsealed,
    kDealStatusWaitForAcceptance,

    /* Waiting for the payment channel creation to complete */
    kDealStatusPaymentChannelCreating,

    /* Waiting for funds to finish being sent to the payment channel */
    kDealStatusPaymentChannelAddingFunds,

    /* Ready to proceed with retrieval */
    kDealStatusAccepted,

    kDealStatusFundsNeededUnseal,

    /* Something went wrong during retrieval */
    kDealStatusFailed,

    /* Provider rejected client's deal proposal */
    kDealStatusRejected,

    /* Provider needs a payment voucher to continue */
    kDealStatusFundsNeeded,

    kDealStatusSendFunds,
    kDealStatusSendFundsLastPayment,

    /* Provider is continuing to process a deal */
    kDealStatusOngoing,

    /* Provider need a last payment voucher to complete a deal */
    kDealStatusFundsNeededLastPayment,

    /* Deal is completed */
    kDealStatusCompleted,

    /* Deal couldn't be identified */
    kDealStatusDealNotFound,

    /* Something went wrong with deal */
    kDealStatusErrored,

    /* All blocks has been processed for the piece */
    kDealStatusBlocksComplete,

    /* Last payment has been received, confirming deal */
    kDealStatusFinalizing,

    kDealStatusCompleting,
    kDealStatusCheckComplete,
    kDealStatusCheckFunds,
    kDealStatusInsufficientFunds,
    kDealStatusPaymentChannelAllocatingLane,
    kDealStatusCancelling,
    kDealStatusCancelled,
    kDealStatusRetryLegacy,
    kDealStatusWaitForAcceptanceLegacy,
  };
}  // namespace fc::markets::retrieval
