/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_RPC_WS_HPP
#define CPP_FILECOIN_CORE_API_RPC_WS_HPP

#include "api/api.hpp"

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace fc::api {
  void serve(Api api,
             boost::asio::io_context &ioc,
             std::string_view ip,
             unsigned short port);
}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_RPC_WS_HPP
