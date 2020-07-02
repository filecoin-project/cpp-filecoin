/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_COMMON_TYPES_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_COMMON_TYPES_HPP

#include <libp2p/host/basic_host/basic_host.hpp>
#include <libp2p/peer/peer_info.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "primitives/address/address.hpp"
#include "storage/ipld/ipld_block.hpp"

namespace fc::markets::retrieval {
  using common::Buffer;
  using libp2p::peer::PeerId;
  using primitives::address::Address;
  using Block = ::fc::storage::ipld::IPLDBlock;
  using Protocol = libp2p::peer::Protocol;
  using HostService = libp2p::Host;
  using ConnectionId = uint64_t;
  using DealID = uint64_t;

  /**
   * @struct Payment info
   */
  struct PaymentInfo {
    Address payment_channel;
    uint64_t lane;
  };

  /**
   * @enum Deal statuses
   */
  enum class DealStatus : uint64_t {
    /* New deal, nothing happened with it */
    kDealStatusNew = 1,

    /* Waiting for the payment channel creation to complete */
    kDealStatusPaymentChannelCreating,

    /* Waiting for funds to finish being sent to the payment channel */
    kDealStatusPaymentChannelAddingFunds,

    /* Waiting for the lane allocation to complete */
    kDealStatusPaymentChannelAllocatingLane,

    /* Payment channel and lane are ready */
    kDealStatusPaymentChannelReady,

    /* Ready to proceed with retrieval */
    kDealStatusAccepted,

    /* Something went wrong during retrieval */
    kDealStatusFailed,

    /* Provider rejected client's deal proposal */
    kDealStatusRejected,

    /* Provider needs a payment voucher to continue */
    kDealStatusFundsNeeded,

    /* Provider is continuing to process a deal */
    kDealStatusOngoing,

    /* Provider need a last payment voucher to complete a deal */
    kDealStatusFundsNeededLastPayment,

    /* Deal is completed */
    kDealStatusCompleted,

    /* Deal couldn't be identified */
    kDealStatusDealNotFound,

    /* Deal had been verified as having right params */
    kDealStatusVerified,

    /* Something went wrong with deal */
    kDealStatusErrored,

    /* All blocks has been processed for the piece */
    kDealStatusBlocksComplete,

    /* Last payment has been received, confirming deal */
    kDealStatusFinalizing

  };
}  // namespace fc::markets::retrieval

#endif
