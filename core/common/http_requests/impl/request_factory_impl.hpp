/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_REQUEST_FACTORY_IMPL_HPP
#define CPP_FILECOIN_REQUEST_FACTORY_IMPL_HPP

#include "common/http_requests/request_factory.hpp"

namespace fc::common {
  class RequestFactoryImpl : public RequestFactory {
   public:
    outcome::result<std::unique_ptr<Request>> newRequest(
        const std::string &url) override;
  };

  enum class RequestFactoryErrors {
    UnableInit = 1,
  };
};  // namespace fc::common

OUTCOME_HPP_DECLARE_ERROR(fc::common, RequestFactoryErrors);

#endif  // CPP_FILECOIN_REQUEST_FACTORY_IMPL_HPP
