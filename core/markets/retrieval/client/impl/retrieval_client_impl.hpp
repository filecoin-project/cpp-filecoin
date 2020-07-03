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
#include "markets/retrieval/client/retrieval_client_error.hpp"

namespace fc::markets::retrieval::client {
  using common::libp2p::CborHost;
  using common::libp2p::CborStream;
  using libp2p::Host;

  /**
   * State of ongoing retrieval deal.
   */
  struct DealState {
    DealProposal proposal;
    std::shared_ptr<CborStream> stream;
    RetrieveResponseHandler handler;
  };

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

    void retrieve(const CID &payload_cid,
                  const DealProposalParams &deal_params,
                  const PeerInfo &provider_peer,
                  const RetrieveResponseHandler &handler) override;

   private:
    void proposeDeal(const DealState &deal_state);

    void setupPaymentChannelStart(const DealState &deal_state);

    void processNextResponse(const DealState &deal_state);

    void processPaymentRequest(const DealState &deal_state);

    void completeDeal(const DealState &deal_state);

    void failDeal(const DealState &deal_state,
                  const RetrievalClientError &error);

    DealId next_deal_id;
    std::shared_ptr<CborHost> host_;
    common::Logger logger_ = common::createLogger("RetrievalMarketClient");
  };

}  // namespace fc::markets::retrieval::client

#endif
