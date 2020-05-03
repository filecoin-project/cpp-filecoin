/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_HOST_CONTEXT_IMPL_HPP
#define CPP_FILECOIN_HOST_CONTEXT_IMPL_HPP

#include "host/context/common_context.hpp"

namespace fc::host::context::common {
  class CommonContextImpl : public CommonContext {
   public:
      CommonContextImpl();

    std::shared_ptr<IoContext> getInputOutput() const override;

    void runInputOutput(size_t seconds) override;

    std::shared_ptr<EventBus> getEventBus() const override;

   private:
    std::shared_ptr<IoContext> io_context_{std::make_shared<IoContext>()};
    std::shared_ptr<SystemSignals> signals_{
        std::make_shared<SystemSignals>(*io_context_, SIGINT, SIGTERM)};
    std::shared_ptr<EventBus> event_bus_{std::make_shared<EventBus>()};
  };
}  // namespace fc::host

#endif
