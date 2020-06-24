/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PAYCHANNEL_MANAGER_PAYCHANNEL_MANAGER_ERROR_HPP
#define CPP_FILECOIN_PAYCHANNEL_MANAGER_PAYCHANNEL_MANAGER_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::paychannel_manager {

  enum class PayChannelManagerError {
    kSendFundsErrored = 1,
    kCreateChannelActorErrored,
    kChannelNotFound,
    kWrongNonce,
    kInsufficientFunds,
    kWrongSignature,
    kAlreadyRedeemed,
  };

};

OUTCOME_HPP_DECLARE_ERROR(fc::paychannel_manager, PayChannelManagerError);

#endif  // CPP_FILECOIN_PAYCHANNEL_MANAGER_PAYCHANNEL_MANAGER_ERROR_HPP
