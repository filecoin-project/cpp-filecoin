/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host/context/network/network_context_impl.hpp"

namespace fc::host::context::network {
  std::shared_ptr<NetworkContext::Host> NetworkContextImpl::createHost() const {
    std::unique_ptr<Network> network = std::make_unique<NetworkImpl>(
        listen_manager_,
        std::make_unique<DialerImpl>(multiselect_,
                                     transport_manager_,
                                     connection_manager_,
                                     listen_manager_),
        connection_manager_);
    std::unique_ptr<PeerRepository> peer_repository =
        std::make_unique<PeerRepositoryImpl>(
            address_repository_, key_repository_, protocol_repository_);
    return std::make_shared<Host>(security_context_->getIdentityManager(),
                                  std::move(network),
                                  std::move(peer_repository),
                                  common_context_->getEventBus());
  }
}  // namespace fc::host::context::network
