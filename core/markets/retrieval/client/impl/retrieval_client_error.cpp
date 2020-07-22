/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/client/retrieval_client_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::retrieval::client,
                            RetrievalClientError,
                            e) {
  using fc::markets::retrieval::client::RetrievalClientError;

  switch (e) {
    case RetrievalClientError::kEarlyTermination:
      return "RetrievalClient: deal terminated early";
    case RetrievalClientError::kResponseDealRejected:
      return "RetrievalClient: deal rejected";
    case RetrievalClientError::kResponseNotFound:
      return "RetrievalClient: piece not found";
    case RetrievalClientError::kUnknownResponseReceived:
      return "RetrievalClient: unknown response";
    case RetrievalClientError::kRequestedTooMuch:
      return "RetrievalClient: amount requested more than left";
    case RetrievalClientError::kBadPaymentRequestBytesNotReceived:
      return "RetrievalClient: not enough bytes received between payment "
             "requests";
    case RetrievalClientError::kBadPaymentRequestTooMuch:
      return "RetrievalClient: too much money requested for bytes sent";
    case RetrievalClientError::kBlockCidParseError:
      return "RetrievalClient: block cid parse error";
    default:
      return "RetrievalClient: unknown error";
  }
}
