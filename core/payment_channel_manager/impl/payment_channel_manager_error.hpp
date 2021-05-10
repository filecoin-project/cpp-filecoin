/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::payment_channel_manager {

  enum class PaymentChannelManagerError {
    kSendFundsErrored = 1,
    kCreateChannelActorErrored,
    kChannelNotFound,
    kWrongNonce,
    kInsufficientFunds,
    kWrongSignature,
    kAlreadyRedeemed,
  };

};

OUTCOME_HPP_DECLARE_ERROR(fc::payment_channel_manager,
                          PaymentChannelManagerError);
