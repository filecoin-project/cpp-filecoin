/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "retrieval_client_impl.hpp"
#include "codec/cbor/cbor.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"

namespace fc::markets::retrieval::client {

  RetrievalClientImpl::RetrievalClientImpl(std::shared_ptr<Host> host)
      : host_{std::make_shared<CborHost>(host)} {}

  outcome::result<std::vector<PeerInfo>> RetrievalClientImpl::findProviders(
      const CID &piece_cid) const {
    return outcome::failure(RetrievalClientError::NOT_IMPLEMENTED);
  }

  void RetrievalClientImpl::query(
      const PeerInfo &peer,
      const QueryRequest &request,
      const QueryResponseHandler &response_handler) {
    host_->newCborStream(
        peer,
        kQueryProtocolId,
        [self{shared_from_this()}, peer, request, response_handler](
            auto stream_res) {
          if (stream_res.has_error()) {
            response_handler(stream_res.error());
            return;
          }
          self->logger_->debug("connected to provider ID "
                               + peerInfoToPrettyString(peer));
          stream_res.value()->write(
              request,
              [self, stream{stream_res.value()}, response_handler](
                  auto written_res) {
                if (written_res.has_error()) {
                  response_handler(written_res.error());
                  return;
                }
                stream->template read<QueryResponse>(
                    [self, response_handler](auto response) {
                      response_handler(response);
                    });
              });
        });
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
