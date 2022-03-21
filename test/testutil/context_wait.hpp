/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>

inline void stopAfterSteps(boost::asio::io_context &io, size_t steps) {
  io.post([&io, steps] {
    if (steps == 0) {
      return io.stop();
    }
    stopAfterSteps(io, steps - 1);
  });
}

inline void runForSteps(boost::asio::io_context &io, size_t steps) {
  stopAfterSteps(io, steps);
  io.run();
}
