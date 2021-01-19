/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_RPC_WS_HPP
#define CPP_FILECOIN_CORE_API_RPC_WS_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include "api/api.hpp"

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace fc::api {
  namespace http = boost::beast::http;
  using tcp = boost::asio::ip::tcp;

  using ResponseType = std::variant<http::response<http::file_body>,
                                    http::response<http::string_body>,
                                    http::response<http::empty_body>>;

  // Wrapper for any type of response
  // This is necessary in order not to lose the response before recording
  class WrapperResponse {
   public:
    ~WrapperResponse();
    WrapperResponse() = default;

    WrapperResponse(ResponseType &&response);

    WrapperResponse(ResponseType &&response, std::function<void()> &&clear);

    WrapperResponse(WrapperResponse &&other) noexcept;

    WrapperResponse &operator=(WrapperResponse &&other) noexcept;

    ResponseType response;
    std::function<void()> release_resources;
  };
  // RouteHandler should write response
  using RouteHandler = std::function<WrapperResponse(
      const http::request<http::dynamic_body> &request)>;
  using Routes = std::map<std::string, RouteHandler, std::greater<>>;

  void serve(std::shared_ptr<Api> api,
             std::shared_ptr<Routes> routes,
             boost::asio::io_context &ioc,
             std::string_view ip,
             unsigned short port);
}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_RPC_WS_HPP
