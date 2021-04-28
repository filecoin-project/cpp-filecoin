/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_LIBP2P_SCHEDULER_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_LIBP2P_SCHEDULER_MOCK_HPP

#include <gmock/gmock.h>

#include <libp2p/protocol/common/scheduler.hpp>

namespace libp2p::protocol {

  class SchedulerMock : public Scheduler {
   public:
    MOCK_CONST_METHOD0(now, Ticks());
    void next_clock() {
      pulse(false);
    }

   protected:
    void scheduleImmediate() override {
      pulse(true);
    }
  };

}  // namespace libp2p::protocol

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_MINER_MINER_MOCK_HPP
