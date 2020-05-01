/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host/context/impl/host_context_impl.hpp"

namespace fc::host {
  HostContextImpl::HostContextImpl() {
    signals_->async_wait([this](const auto &error, int code) {
      if (!error) {
        this->getIoContext()->stop();
      }
    });
  }

  std::shared_ptr<HostContext::IoContext> HostContextImpl::getIoContext() const {
    return io_context_;
  }

  void HostContextImpl::runIoContext(size_t seconds) {
    if (seconds == 0) {
      io_context_->run();
    } else {
      io_context_->run_for(std::chrono::seconds(seconds));
    }
  }
}  // namespace fc::host
