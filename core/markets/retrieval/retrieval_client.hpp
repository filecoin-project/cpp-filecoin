/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_HPP

#include <vector>

#include <libp2p/peer/peer_info.hpp>
#include "common/outcome.hpp"
#include "markets/retrieval/client/retrieval_client_types.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"
#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::markets::retrieval::client {
  /*
   * @class Retrieval market client
   */
  class RetrievalClient {
   protected:
    using Peer = libp2p::peer::PeerInfo;
    using QueryResponseShPtr = std::shared_ptr<QueryResponse>;

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
    virtual outcome::result<std::vector<Peer>> findProviders(
        const CID &piece_cid) const = 0;

    /**
     * @brief Query selected provider
     * @param peer - retrieval peer to query
     * @param request - query params for the provider
     * @return Query response
     */
    virtual outcome::result<QueryResponseShPtr> query(
        const Peer &peer, const QueryRequest &request) = 0;

    /**
     * @brief Retrieve Piece from selected provider
     * @param piece_cid - identifier of the Piece to retrieve
     * @param provider_peer - provider to make a deal
     * @param deal_profile - deal properties
     * @return Identifier of the deal or error
     */
    virtual outcome::result<std::vector<Block>> retrieve(
        const CID &piece_cid,
        const Peer &provider_peer,
        const DealProfile &deal_profile) = 0;
  };

  /**
   * @enum Retrieval client errors
   */
  enum class RetrievalClientError { NOT_IMPLEMENTED };
}  // namespace fc::markets::retrieval::client

OUTCOME_HPP_DECLARE_ERROR(fc::markets::retrieval::client, RetrievalClientError);

#endif
