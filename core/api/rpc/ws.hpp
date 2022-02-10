/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <map>
#include <variant>

#include "api/rpc/rpc.hpp"
#include "primitives/jwt/jwt.hpp"

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace fc::api {
  namespace http = boost::beast::http;
  using tcp = boost::asio::ip::tcp;
  using rpc::AuthFunction;
  using rpc::Permissions;
  using rpc::Rpc;

  using ResponseType = std::variant<http::response<http::file_body>,
                                    http::response<http::string_body>,
                                    http::response<http::empty_body>>;

  // Wrapper for any type of response
  // This is necessary in order not to lose the response before recording
  class WrapperResponse {
   public:
    WrapperResponse() = default;

    WrapperResponse(const WrapperResponse &) = delete;

    explicit WrapperResponse(ResponseType &&response);

    WrapperResponse(ResponseType &&response, std::function<void()> &&clear);

    WrapperResponse(WrapperResponse &&other) noexcept;

    ~WrapperResponse();

    WrapperResponse &operator=(const WrapperResponse &other) = delete;

    WrapperResponse &operator=(WrapperResponse &&other) noexcept;

    ResponseType response;
    std::function<void()> release_resources;
  };

  api::WrapperResponse makeErrorResponse(
      const http::request<http::string_body> &request, http::status status);

  using RouteCB = std::function<void(WrapperResponse &&)>;
  using RouteHandler = std::function<void(
      const http::request<http::string_body> &request, const RouteCB &)>;
  using AuthRouteHandler =
      std::function<void(const http::request<http::string_body> &request,
                         const Permissions &perms,
                         const RouteCB &)>;
  RouteHandler makeAuthRoute(AuthRouteHandler &&handler, AuthFunction &&auth);
  AuthRouteHandler makeHttpRpc(std::shared_ptr<Rpc> rpc);

  using Routes = std::map<std::string, RouteHandler, std::greater<>>;

  void serve(const std::map<std::string, std::shared_ptr<Rpc>> &rpc,
             const std::shared_ptr<Routes> &routes,
             boost::asio::io_context &ioc,
             std::string_view ip,
             unsigned short port);
}  // namespace fc::api
