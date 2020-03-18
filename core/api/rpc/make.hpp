/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_RPC_MAKE_HPP
#define CPP_FILECOIN_CORE_API_RPC_MAKE_HPP

#include "api/api.hpp"
#include "api/rpc/rpc.hpp"

namespace fc::api {
  using rpc::Rpc;

  void setupRpc(Rpc &rpc, const Api &api);
}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_RPC_MAKE_HPP
