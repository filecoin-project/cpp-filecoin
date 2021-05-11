/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
