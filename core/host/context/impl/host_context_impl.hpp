/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_HOST_CONTEXT_IMPL_HPP
#define CPP_FILECOIN_HOST_CONTEXT_IMPL_HPP

#include "host/context/host_context.hpp"

namespace fc::host {
  class HostContextImpl : public HostContext {
   public:
    HostContextImpl();

    HostContextImpl(std::shared_ptr<IoContext> context);

    std::shared_ptr<IoContext> getIoContext() const override;

    void runIoContext(size_t seconds) override;

   private:
    std::shared_ptr<IoContext> io_context_{std::make_shared<IoContext>()};
    std::shared_ptr<SystemSignals> signals_{
        std::make_shared<SystemSignals>(*io_context_, SIGINT, SIGTERM)};
  };
}  // namespace fc::host

#endif
