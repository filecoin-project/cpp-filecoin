/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_RPC_WS_HPP
#define CPP_FILECOIN_CORE_API_RPC_WS_HPP

#include <boost/beast/http.hpp>
#include "api/api.hpp"

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace fc::api {
  namespace beast = boost::beast;
  namespace http = beast::http;

  using RouteHandler =
      std::function<void(const http::request<http::dynamic_body> &request,
                         http::response<http::dynamic_body> &response)>;
  using Routes = std::map<std::string, RouteHandler, std::greater<>>;

  void serve(std::shared_ptr<Api> api,
             std::shared_ptr<Routes> routes,
             boost::asio::io_context &ioc,
             std::string_view ip,
             unsigned short port);
}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_RPC_WS_HPP
