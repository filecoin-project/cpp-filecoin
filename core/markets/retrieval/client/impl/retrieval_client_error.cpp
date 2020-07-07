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
      return "RetrievalClientError: deal terminated early";
    case RetrievalClientError::kUnknownResponseReceived:
      return "RetrievalClientError: unknown response";
  }

  return "RetrievalClientError: unknown error";
}
