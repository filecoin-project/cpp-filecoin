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
#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor_state.hpp"

namespace fc::markets::retrieval {
  using primitives::address::Address;
  using vm::actor::builtin::v0::payment_channel::LaneId;

  /**
   * @struct Payment info
   */
  struct PaymentInfo {
    Address payment_channel;
    LaneId lane;
  };

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

#endif
