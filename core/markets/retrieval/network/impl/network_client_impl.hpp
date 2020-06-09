/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_RETRIEVAL_NETWORK_CLIENT_IMPL_HPP
#define CPP_FILECOIN_MARKETS_RETRIEVAL_NETWORK_CLIENT_IMPL_HPP

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/basic_host/basic_host.hpp>
#include "markets/retrieval/network/async_operation.hpp"
#include "markets/retrieval/network/network_client.hpp"
#include "markets/retrieval/types.hpp"

namespace fc::markets::retrieval::network {
  class NetworkClientImpl : public NetworkClient {
   public:
    using Async = AsyncOperation;
    using Operation = AsyncOperation::Operation;

    NetworkClientImpl(std::shared_ptr<HostService> service)
        : host_service_{std::move(service)} {}

    outcome::result<StreamShPtr> connect(const PeerInfo &peer,
                                         const Protocol &proto) override;

   private:
    std::shared_ptr<HostService> host_service_;

    void openStream(Operation operation,
                    const PeerInfo &peer,
                    const Protocol &proto,
                    StreamShPtr &stream);
  };

}  // namespace fc::markets::retrieval::network

#endif
