/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "retrieval_client_impl.hpp"
#include "codec/cbor/cbor.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"

#define IF_ERROR_RETURN(result, handler) \
  if (result.has_error()) {              \
    handler(result.error());             \
    return;                              \
  }

namespace fc::markets::retrieval::client {

  RetrievalClientImpl::RetrievalClientImpl(std::shared_ptr<Host> host)
      : host_{std::make_shared<CborHost>(host)} {}

  outcome::result<std::vector<PeerInfo>> RetrievalClientImpl::findProviders(
      const CID &piece_cid) const {
    // TODO (a.chernyshov) implement
    return outcome::failure(RetrievalClientError::kUnknownResponseReceived);
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
          IF_ERROR_RETURN(stream_res, response_handler);
          self->logger_->debug("connected to provider ID "
                               + peerInfoToPrettyString(peer));
          stream_res.value()->write(
              request,
              [stream{stream_res.value()}, response_handler](auto written_res) {
                IF_ERROR_RETURN(written_res, response_handler);
                stream->template read<QueryResponse>(
                    [response_handler](auto response) {
                      response_handler(response);
                    });
              });
        });
  }

  void RetrievalClientImpl::retrieve(const CID &payload_cid,
                                     const DealProposalParams &deal_params,
                                     const PeerInfo &provider_peer,
                                     const RetrieveResponseHandler &handler) {
    DealProposal proposal{.payload_cid = payload_cid,
                          .deal_id = next_deal_id++,
                          .params = deal_params};
    host_->newCborStream(
        provider_peer,
        kRetrievalProtocolId,
        [self{shared_from_this()}, proposal, handler](auto stream_res) {
          IF_ERROR_RETURN(stream_res, handler);
          self->proposeDeal(stream_res.value(), proposal, handler);
        });
  }

  void RetrievalClientImpl::proposeDeal(
      const std::shared_ptr<CborStream> &stream,
      const DealProposal &proposal,
      const RetrieveResponseHandler &handler) {
    stream->write(
        proposal, [self{shared_from_this()}, stream, handler](auto written) {
          IF_ERROR_RETURN(written, handler);
          stream->template read<DealResponse>([self, stream, handler](
                                                  auto response) {
            IF_ERROR_RETURN(response, handler);
            if (response.value().status == DealStatus::kDealStatusAccepted) {
              self->setupPaymentChannelStart(stream, handler);
            } else {
              // TODO (a.chernyshov) handle not accepted status
            }
          });
        });
  }

  void RetrievalClientImpl::setupPaymentChannelStart(
      const std::shared_ptr<CborStream> &stream,
      const RetrieveResponseHandler &handler) {
    // TODO (a.chernyshov) api->GetOrCreatePaymentChannel
    // wait for create and funding
    // allocate lane - ???
    processNextResponse(stream, handler);
  }

  void RetrievalClientImpl::processNextResponse(
      const std::shared_ptr<CborStream> &stream,
      const RetrieveResponseHandler &handler) {
    // TODO not clear - 2 responses one by one?
    stream->read<DealResponse>(
        [self{shared_from_this()}, stream, handler](auto response) {
          IF_ERROR_RETURN(response, handler);

          // TODO consume blocks
          bool completed = true;

          if (completed) {
            switch (response.value().status) {
              case DealStatus::kDealStatusFundsNeededLastPayment:
                self->processPaymentRequest(stream, handler);
                break;
              case DealStatus::kDealStatusBlocksComplete:
                // TODO check this status
                self->processNextResponse(stream, handler);
                break;
              case DealStatus::kDealStatusCompleted:
                self->completeDeal(stream, handler);
                break;
              default:
                self->failDeal(stream,
                               handler,
                               RetrievalClientError::kUnknownResponseReceived);
            }
          } else {
            switch (response.value().status) {
              case DealStatus::kDealStatusFundsNeededLastPayment:
              case DealStatus::kDealStatusBlocksComplete:
                self->failDeal(
                    stream, handler, RetrievalClientError::kEarlyTermination);
                break;
              case DealStatus::kDealStatusFundsNeeded:
                self->processPaymentRequest(stream, handler);
                break;
              case DealStatus::kDealStatusOngoing:
                self->processNextResponse(stream, handler);
                break;
              default:
                self->failDeal(stream,
                               handler,
                               RetrievalClientError::kUnknownResponseReceived);
            }
          }
        });
  }

  void RetrievalClientImpl::processPaymentRequest(
      const std::shared_ptr<CborStream> &stream,
      const RetrieveResponseHandler &handler) {}

  void RetrievalClientImpl::completeDeal(
      const std::shared_ptr<CborStream> &stream,
      const RetrieveResponseHandler &handler) {
    if (!stream->stream()->isClosed()) {
      stream->stream()->close(handler);
    }
  }

  void RetrievalClientImpl::failDeal(const std::shared_ptr<CborStream> &stream,
                                     const RetrieveResponseHandler &handler,
                                     const RetrievalClientError &error) {
    if (!stream->stream()->isClosed()) {
      stream->stream()->close(handler);
    }
    handler(error);
  }

}  // namespace fc::markets::retrieval::client
