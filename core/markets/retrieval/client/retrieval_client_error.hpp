/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_RETRIEVAL_CLIENT_ERROR_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_RETRIEVAL_CLIENT_ERROR_HPP

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

#endif  // CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_RETRIEVAL_CLIENT_ERROR_HPP
