/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host/context/common/common_context_impl.hpp"

namespace fc::host::context::common {
  CommonContextImpl::CommonContextImpl() {
    signals_->async_wait([this](const auto &error, int code) {
      if (!error) {
        this->getInputOutput()->stop();
      }
    });
  }

  std::shared_ptr<CommonContextImpl::IoContext>
  CommonContextImpl::getInputOutput() const {
    return io_context_;
  }

  void CommonContextImpl::runInputOutput(size_t seconds) {
    if (seconds == 0) {
      io_context_->run();
    } else {
      io_context_->run_for(std::chrono::seconds(seconds));
    }
  }

  std::shared_ptr<CommonContextImpl::EventBus> CommonContextImpl::getEventBus()
      const {
    return event_bus_;
  }
}  // namespace fc::host::context::common
