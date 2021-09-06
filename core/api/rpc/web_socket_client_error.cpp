/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/web_socket_client_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::api::rpc, WebSocketClientError, e) {
  using fc::api::rpc::WebSocketClientError;

  if (e == WebSocketClientError::kRpcErrorResponse) {
    return "RPC error: got error response";
  }
  return "unknown error";
}
