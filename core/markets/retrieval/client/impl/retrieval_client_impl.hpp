/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_IMPL_HPP

#include <memory>

#include <libp2p/host/host.hpp>
#include "common/logger.hpp"
#include "markets/retrieval/client/retrieval_client.hpp"

namespace fc::markets::retrieval::client {
  using common::libp2p::CborHost;
  using libp2p::Host;

  class RetrievalClientImpl
      : public RetrievalClient,
        public std::enable_shared_from_this<RetrievalClientImpl> {
   public:
    /**
     * @brief Constructor
     * @param host - libp2p network backend
     */
    RetrievalClientImpl(std::shared_ptr<Host> host);

    outcome::result<std::vector<PeerInfo>> findProviders(
        const CID &piece_cid) const override;

    void query(const PeerInfo &peer,
               const QueryRequest &request,
               const QueryResponseHandler &response_handler) override;

    outcome::result<std::vector<Block>> retrieve(
        const CID &piece_cid,
        const PeerInfo &provider_peer,
        const DealProfile &deal_profile) override;

   private:
    std::shared_ptr<CborHost> host_;
    common::Logger logger_ = common::createLogger("RetrievalMarketClient");
  };

}  // namespace fc::markets::retrieval::client

#endif
