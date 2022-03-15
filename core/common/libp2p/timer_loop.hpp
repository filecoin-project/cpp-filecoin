/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/scheduler.hpp>

#include "common/ptr.hpp"

namespace fc {
  template <typename Cb>
  void timerLoop(const std::shared_ptr<libp2p::basic::Scheduler> &scheduler,
                 const std::chrono::milliseconds &interval,
                 Cb &&cb) {
    scheduler->schedule(
        weakCb(scheduler,
               [interval, cb{std::forward<Cb>(cb)}](
                   std::shared_ptr<libp2p::basic::Scheduler> &&scheduler) {
                 cb();
                 timerLoop(scheduler, interval, std::move(cb));
               }),
        interval);
  }
}  // namespace fc
