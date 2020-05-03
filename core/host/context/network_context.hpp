/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FUHON_CORE_HOST_CONTEXT_NETWORK_HPP
#define CPP_FUHON_CORE_HOST_CONTEXT_NETWORK_HPP

#include <memory>

#include <libp2p/host/basic_host/basic_host.hpp>

namespace fc::host::context::network {
  class NetworkContext {
   protected:
    using Host = libp2p::host::BasicHost;

   public:
    virtual ~NetworkContext() = default;

    virtual std::shared_ptr<Host> createHost() const = 0;
  };
}  // namespace fc::host::context::network
#endif
