/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_IMPL_HPP

#include <memory>

#include <libp2p/host/basic_host/basic_host.hpp>
#include "common/logger.hpp"
#include "markets/retrieval/network/impl/network_client_impl.hpp"
#include "markets/retrieval/retrieval_client.hpp"

namespace fc::markets::retrieval::client {
  class RetrievalClientImpl : public RetrievalClient,
                              network::NetworkClientImpl {
   protected:
    using HostService = libp2p::Host;

   public:
    /**
     * @brief Constructor
     * @param p2p_host - network backend
     */
    RetrievalClientImpl(std::shared_ptr<HostService> p2p_host)
        : network::NetworkClientImpl{std::move(p2p_host)},
          logger_{common::createLogger(
              "Retrieval client")} {}

    outcome::result<std::vector<PeerInfo>> findProviders(
        const CID &piece_cid) const override;

    outcome::result<QueryResponseShPtr> query(const PeerInfo &peer,
                                         const QueryRequest &request) override;

    outcome::result<std::vector<Block>> retrieve(
        const CID &piece_cid,
        const PeerInfo &provider_peer,
        const DealProfile &deal_profile) override;

   private:
    std::shared_ptr<HostService> host_service_;
    common::Logger logger_;
  };
}  // namespace fc::markets::retrieval::client

#endif
