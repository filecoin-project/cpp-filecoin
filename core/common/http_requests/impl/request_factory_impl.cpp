/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/http_requests/impl/request_factory_impl.hpp"

#include "common/http_requests/impl/request_impl.hpp"

namespace fc::common {

  outcome::result<std::unique_ptr<Request>> RequestFactoryImpl::newRequest(
      const std::string &url) {
    struct make_unique_enabler : public RequestImpl {
      make_unique_enabler() : RequestImpl{} {};
    };

    std::unique_ptr<RequestImpl> request =
        std::make_unique<make_unique_enabler>();

    if (!request->curl_) {
      return RequestFactoryErrors::UnableInit;
    }

    request->setupUrl(url);

    return outcome::success(std::move(request));
  }
}  // namespace fc::common

OUTCOME_CPP_DEFINE_CATEGORY(fc::common, RequestFactoryErrors, e) {
  using fc::common::RequestFactoryErrors;
  switch (e) {
    case (RequestFactoryErrors::UnableInit):
      return "RequestFactory: Unable to init a request";
    default:
      return "RequestFactory: unknown error";
  }
}
