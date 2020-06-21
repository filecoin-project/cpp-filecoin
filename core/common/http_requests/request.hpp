/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_HTTP_REQUESTS_REQUEST_HPP
#define CPP_FILECOIN_CORE_COMMON_HTTP_REQUESTS_REQUEST_HPP

#include <string>
#include <unordered_map>
#include "common/outcome.hpp"

namespace fc::common {

  using HeaderName = std::string;
  using HeaderValue = std::string;

  enum ReqMethod {
    GET,
    PUT,
    POST,
    DELETE,
  };

  struct Response {
    long status_code;
    std::string content_type;
  };

  class Request {
   public:
    virtual ~Request() = default;

    virtual void setupWriteFunction(std::size_t (*callback)(
        const char *, std::size_t, std::size_t, void *)) = 0;

    virtual void setupWriteOutput(void *output) = 0;

    virtual void setupUrl(const std::string &url) = 0;

    virtual void setupMethod(ReqMethod method) = 0;

    virtual void setupHeaders(
        const std::unordered_map<HeaderName, HeaderValue> &headers) = 0;

    virtual void setupHeader(
        const std::pair<HeaderName, HeaderValue> &header) = 0;

    virtual Response perform() = 0;
  };

}  // namespace fc::common

#endif  // CPP_FILECOIN_CORE_COMMON_HTTP_REQUESTS_REQUEST_HPP
