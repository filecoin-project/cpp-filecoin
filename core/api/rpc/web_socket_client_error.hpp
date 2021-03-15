/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::api::rpc {

  /**
   * @brief Type of errors returned by StateTree
   */
  enum class WebSocketClientError {
    kRpcErrorResponse = 1,
  };

}  // namespace fc::api::rpc

OUTCOME_HPP_DECLARE_ERROR(fc::api::rpc, WebSocketClientError);
