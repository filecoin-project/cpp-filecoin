/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_RPC_JSON_ERRORS_HPP
#define CPP_FILECOIN_CORE_API_RPC_JSON_ERRORS_HPP

#include "common/outcome.hpp"

namespace fc::api {
  enum class JsonError {
    kWrongLength = 1,
    kWrongEnum,
    kWrongType,
    kOutOfRange,
    kWrongParams,
  };
}  // namespace fc::api

OUTCOME_HPP_DECLARE_ERROR(fc::api, JsonError);

#endif  // CPP_FILECOIN_CORE_API_RPC_JSON_ERRORS_HPP
