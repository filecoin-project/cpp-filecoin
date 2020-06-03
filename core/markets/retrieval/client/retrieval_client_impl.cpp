/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/client/retrieval_client_impl.hpp"
#include "codec/cbor/cbor.hpp"
#include "markets/retrieval/network/impl/network_client_impl.hpp"
#include "markets/retrieval/network/sync_cbor_stream.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"

namespace fc::markets::retrieval::client {
  using PeerInfo = network::NetworkClient::PeerInfo;

  outcome::result<std::vector<PeerInfo>> RetrievalClientImpl::findProviders(
      const CID &piece_cid) const {
    return outcome::failure(RetrievalClientError::NOT_IMPLEMENTED);
  }

  outcome::result<RetrievalClient::QueryResponseShPtr>
  RetrievalClientImpl::query(const PeerInfo &peer,
                             const QueryRequest &request) {
    OUTCOME_TRY(stream, connect(peer, kQueryProtocolId));
    logger_->info("connected to provider ID " + peer.id.toBase58());
    auto sync_stream = std::make_shared<network::SyncCborStream>(stream);
    OUTCOME_TRY(sync_stream->write(request));
    OUTCOME_TRY(response, sync_stream->read<QueryResponse>());
    logger_->info("received QueryResponse for "
                  + request.payload_cid.toString().value());
    return std::move(response);

    /*
    OUTCOME_TRY(encoded_request, codec::cbor::encode(request));
    OUTCOME_TRY(stream, connect(peer, kQueryProtocolId));
    logger_->info("connected to provider ID " + peer.id.toBase58());
    OUTCOME_TRY(stream->write(encoded_request));
    OUTCOME_TRY(encoded_response, stream->read());
    OUTCOME_TRY(response, codec::cbor::decode<QueryResponse>(encoded_response));
    OUTCOME_TRY(stream->close());
    logger_->info("received QueryResponse for " +
    request.payload_cid.toString().value()); return std::move(response);
     */
  }

  outcome::result<std::vector<Block>> RetrievalClientImpl::retrieve(
      const CID &piece_cid,
      const PeerInfo &provider_peer,
      const DealProfile &deal_profile) {
    return std::vector<Block>{};
  }
}  // namespace fc::markets::retrieval::client

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::retrieval::client,
                            RetrievalClientError,
                            e) {
  return "NOT IMPLEMENTED";
}
