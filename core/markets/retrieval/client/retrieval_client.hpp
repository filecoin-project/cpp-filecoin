/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include <libp2p/peer/peer_info.hpp>
#include "common/outcome.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"
#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::markets::retrieval::client {
  using libp2p::peer::PeerInfo;
  using QueryResponseHandler =
      std::function<void(outcome::result<QueryResponseV1_0_0>)>;
  using RetrieveResponseHandler = std::function<void(outcome::result<void>)>;

  /*
   * @class Retrieval market client
   */
  class RetrievalClient {
   public:
    /**
     * @brief Destructor
     */
    virtual ~RetrievalClient() = default;

    /**
     * @brief Find providers, which has requested Piece
     * @param piece_cid - identifier of the requested Piece
     * @return Providers, which has requested piece
     */
    virtual outcome::result<std::vector<PeerInfo>> findProviders(
        const CID &piece_cid) const = 0;

    /**
     * @brief Query selected provider
     * @param peer - retrieval peer to query
     * @param request - query params for the provider
     * @param Query response handler
     */
    virtual void query(const RetrievalPeer &provider_peer,
                       const QueryRequest &request,
                       const QueryResponseHandler &cb) = 0;

    /**
     * @brief Retrieve Piece from selected provider
     * @param payload_cid - identifier of the data to retrieve
     * @param deal_params - deal properties
     * @param total_funds - funds for deal
     * @param provider_peer - provider to make a deal
     * @param client_wallet - client wallet to send funds for deal from
     * @param miner_wallet - miner wallet to pay to
     * @param handler - deal response handler, called on error or completion
     */
    virtual outcome::result<void> retrieve(
        const CID &payload_cid,
        const DealProposalParams &deal_params,
        const TokenAmount &total_funds,
        const RetrievalPeer &provider_peer,
        const Address &client_wallet,
        const Address &miner_wallet,
        const RetrieveResponseHandler &handler) = 0;
  };

}  // namespace fc::markets::retrieval::client
