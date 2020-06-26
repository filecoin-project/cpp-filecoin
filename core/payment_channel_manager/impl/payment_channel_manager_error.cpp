/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "payment_channel_manager/impl/payment_channel_manager_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::payment_channel_manager,
                            PaymentChannelManagerError,
                            e) {
  using E = fc::payment_channel_manager::PaymentChannelManagerError;
  switch (e) {
    case E::kSendFundsErrored:
      return "PayChannelManager: error code on send funds returned";
    case E::kCreateChannelActorErrored:
      return "PayChannelManager: error code on create payment channel actor";
    case E::kChannelNotFound:
      return "PayChannelManager: channel not found for given address";
    case E::kWrongNonce:
      return "PayChannelManager: voucher nonce is wrong";
    case E::kInsufficientFunds:
      return "PayChannelManager: not enough funds";
    case E::kWrongSignature:
      return "PayChannelManager: wrong signature";
    case E::kAlreadyRedeemed:
      return "PayChannelManager: amount already redeemed";
  }
}
