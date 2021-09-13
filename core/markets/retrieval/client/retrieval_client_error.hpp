/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::markets::retrieval::client {

  /**
   * @brief Type of errors returned by Storage Market Client
   */
  enum class RetrievalClientError {
    kEarlyTermination = 1,
    kResponseDealRejected,
    kResponseNotFound,
    kUnknownResponseReceived,
    kRequestedTooMuch,
    kBadPaymentRequestBytesNotReceived,
    kBadPaymentRequestTooMuch,
    kBlockCidParseError,
  };

}  // namespace fc::markets::retrieval::client

OUTCOME_HPP_DECLARE_ERROR(fc::markets::retrieval::client, RetrievalClientError);
