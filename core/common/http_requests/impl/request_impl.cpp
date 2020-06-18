/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/http_requests/impl/request_impl.hpp"

namespace fc::common {
  void RequestImpl::setupWriteFunction(
      const std::function<std::size_t(
          const char *, std::size_t, std::size_t, std::string *)> &callback) {
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &callback);
  }

  void RequestImpl::setupWriteOutput(void *output) {
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, output);
  }

  void RequestImpl::setupUrl(const std::string &url) {
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
  }

  void RequestImpl::setupMethod(ReqMethod method) {
    switch (method) {
      case ReqMethod::GET:
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "GET");
        return;
      case ReqMethod::DELETE:
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
        return;
      case ReqMethod::POST:
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "POST");
        return;
      case ReqMethod::PUT:
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "PUT");
        return;
      default:
        return;
    }
  }

  void RequestImpl::setupHeaders(
      const std::unordered_map<std::string, std::string> &headers) {
    for (const auto &header : headers) {
      setupHeader(header);
    }
  }

  void RequestImpl::setupHeader(
      const std::pair<std::string, std::string> &header) {
    headers_ = curl_slist_append(headers_,
                                 (header.first + ": " + header.second).c_str());
  }

  Response RequestImpl::perform() {
    Response res;

    if (headers_) {
      curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
    }
    curl_easy_perform(curl_);
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &res.status_code);
    curl_easy_getinfo(curl_, CURLINFO_CONTENT_TYPE, &res.content_type);

    return res;
  }

  RequestImpl::~RequestImpl() {
    if (curl_) {
      curl_easy_cleanup(curl_);
    }

    if (headers_) {
      curl_slist_free_all(headers_);
    }

    Request::~Request();
  }

  RequestImpl::RequestImpl() : headers_(nullptr), curl_(curl_easy_init()) {
    if (curl_) {
      curl_easy_setopt(curl_, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

      // Follow HTTP redirects if necessary
      curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    }
  }

}  // namespace fc::common
