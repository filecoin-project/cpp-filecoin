/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_REQUEST_FACTORY_HPP
#define CPP_FILECOIN_REQUEST_FACTORY_HPP

#include <string>
#include "common/http_requests/request.hpp"

namespace fc::common {
  class RequestFactory {
   public:
    virtual ~RequestFactory() = default;

    virtual outcome::result<std::unique_ptr<Request>> newRequest(
        const std::string &url) = 0;
  };
};  // namespace fc::common

#endif  // CPP_FILECOIN_REQUEST_FACTORY_HPP
