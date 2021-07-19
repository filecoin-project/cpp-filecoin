/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#define MOCK_API(_api, method)                            \
  ::fc::api::Mock<decltype(_api->method)> mock_##method { \
    _api->method                                          \
  }

namespace fc::api {
  template <typename A>
  struct Mock : testing::MockFunction<typename A::FunctionSignature> {
    Mock(std::function<typename A::FunctionSignature> &a) {
      a = this->AsStdFunction();
    }
  };
}  // namespace fc::api
