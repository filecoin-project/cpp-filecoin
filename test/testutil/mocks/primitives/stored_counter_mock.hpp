/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/stored_counter/stored_counter.hpp"

#include <gmock/gmock.h>

namespace fc::primitives {

  class CounterMock : public Counter {
   public:
    MOCK_METHOD0(next, outcome::result<uint64_t>());
  };

}  // namespace fc::primitives
