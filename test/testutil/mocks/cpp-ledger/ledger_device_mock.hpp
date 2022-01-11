/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "cpp-ledger/ledger/ledger.hpp"

namespace ledger {

  class MockLedgerDevice : public LedgerDevice {
   public:
    MOCK_CONST_METHOD1(Exchange,
                       std::tuple<Bytes, Error>(const Bytes &command));
    MOCK_METHOD0(Close, void());
  };

}  // namespace ledger
