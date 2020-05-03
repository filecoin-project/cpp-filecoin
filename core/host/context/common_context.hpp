/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_COMMON_CONTEXT_HPP
#define CPP_FILECOIN_COMMON_CONTEXT_HPP

#include <memory>

#include <boost/asio.hpp>
#include <libp2p/event/bus.hpp>

namespace fc::host::context::common {
  /**
   * @class Host environment
   */
  class CommonContext {
   public:
    using IoContext = boost::asio::io_context;
    using SystemSignals = boost::asio::signal_set;
    using EventBus = libp2p::event::Bus;

    /**
     * @brief Destructor
     */
    virtual ~CommonContext() = default;

    /**
     * @brief Get IO context
     * @return current IO context
     */
    virtual std::shared_ptr<IoContext> getInputOutput() const = 0;

    /**
     * @brief Run IO context for specified time
     * @param seconds - time to run, if 0 - forever
     */
    virtual void runInputOutput(size_t seconds = 0) = 0;

    /**
     * @brief Get event bus
     * @return operation result
     */
    virtual std::shared_ptr<EventBus> getEventBus() const = 0;
  };
}  // namespace fc::host::context::common

#endif
