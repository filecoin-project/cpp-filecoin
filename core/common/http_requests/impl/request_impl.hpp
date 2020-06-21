/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_HTTP_REQUESTS_IMPL_REQUEST_IMPL_HPP
#define CPP_FILECOIN_CORE_COMMON_HTTP_REQUESTS_IMPL_REQUEST_IMPL_HPP

#include "common/http_requests/request.hpp"

#include <curl/curl.h>

namespace fc::common {

  class RequestFactoryImpl;

  class RequestImpl : public Request {
   public:
    ~RequestImpl() override;

    void setupWriteFunction(std::size_t (*callback)(
        const char *, std::size_t, std::size_t, void *)) override;

    void setupWriteOutput(void *output) override;

    void setupUrl(const std::string &url) override;

    void setupMethod(ReqMethod method) override;

    void setupHeaders(
        const std::unordered_map<std::string, std::string> &headers) override;

    void setupHeader(
        const std::pair<std::string, std::string> &header) override;

    Response perform() override;

   private:
    RequestImpl();
    friend class RequestFactoryImpl;

    struct curl_slist *headers_;
    CURL *curl_;
  };

}  // namespace fc::common

#endif  // CPP_FILECOIN_CORE_COMMON_HTTP_REQUESTS_IMPL_REQUEST_IMPL_HPP
