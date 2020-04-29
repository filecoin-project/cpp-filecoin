/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_HOST_CONTEXT_HPP
#define CPP_FILECOIN_HOST_CONTEXT_HPP

#include <memory>

#include <boost/asio.hpp>

namespace fc::host {
  /**
   * @class Host environment
   */
  class HostContext {
   public:
    using IoContext = boost::asio::io_context;
    using SystemSignals = boost::asio::signal_set;

    /**
     * @brief Destructor
     */
    virtual ~HostContext() = default;

    /**
     * @brief Get IO context
     * @return current IO context
     */
    virtual std::shared_ptr<IoContext> getIoContext() const = 0;

    /**
     * @brief Run IO context for specified time
     * @param seconds - time to run, if 0 - forever
     */
    virtual void runIoContext(size_t seconds = 0) = 0;
  };
}  // namespace fc::host

#endif
