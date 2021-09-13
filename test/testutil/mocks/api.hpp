/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include "api/utils.hpp"

#define MOCK_API(_api, method)                            \
  ::fc::api::Mock<decltype(_api->method)> mock_##method { \
    _api->method                                          \
  }

#define MOCK_API_CB(_api, method)                           \
  ::fc::api::MockCb<decltype(_api->method)> mock_##method { \
    _api->method                                            \
  }

namespace fc::api {
  template <typename A>
  struct Mock : testing::MockFunction<typename A::FunctionSimpleSignature> {
    Mock(A &a) {
      a = this->AsStdFunction();
    }
  };

  template <typename A>
  struct MockCb : testing::MockFunction<typename A::FunctionSignature> {
    MockCb(A &a) {
      a = this->AsStdFunction();
    }
  };
}  // namespace fc::api
