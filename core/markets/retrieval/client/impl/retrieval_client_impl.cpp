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
          DealState deal_state{.proposal = proposal,
                               .stream = stream_res.value(),
                               .handler = handler};
          self->proposeDeal(deal_state);
        });
  }

  void RetrievalClientImpl::proposeDeal(const DealState &deal_state) {
    deal_state.stream->write(
        deal_state.proposal,
        [self{shared_from_this()}, deal_state](auto written) {
          IF_ERROR_RETURN(written, deal_state.handler);
          deal_state.stream->template read<DealResponse>([self, deal_state](
                                                             auto response) {
            IF_ERROR_RETURN(response, deal_state.handler);
            if (response.value().status == DealStatus::kDealStatusAccepted) {
              self->setupPaymentChannelStart(deal_state);
            } else {
              // TODO (a.chernyshov) handle not accepted status
            }
          });
        });
  }

  void RetrievalClientImpl::setupPaymentChannelStart(
      const DealState &deal_state) {
    // TODO (a.chernyshov) api->GetOrCreatePaymentChannel
    // wait for create and funding
    // allocate lane - ???
    processNextResponse(deal_state);
  }

  void RetrievalClientImpl::processNextResponse(const DealState &deal_state) {
    // TODO not clear - 2 responses one by one?
    deal_state.stream->read<DealResponse>([self{shared_from_this()},
                                           deal_state](auto response) {
      IF_ERROR_RETURN(response, deal_state.handler);

      // TODO consume blocks
      bool completed = true;

      if (completed) {
        switch (response.value().status) {
          case DealStatus::kDealStatusFundsNeededLastPayment:
            self->processPaymentRequest(deal_state);
            break;
          case DealStatus::kDealStatusBlocksComplete:
            // TODO check this status
            self->processNextResponse(deal_state);
            break;
          case DealStatus::kDealStatusCompleted:
            self->completeDeal(deal_state);
            break;
          default:
            self->failDeal(deal_state,
                           RetrievalClientError::kUnknownResponseReceived);
        }
      } else {
        switch (response.value().status) {
          case DealStatus::kDealStatusFundsNeededLastPayment:
          case DealStatus::kDealStatusBlocksComplete:
            self->failDeal(deal_state, RetrievalClientError::kEarlyTermination);
            break;
          case DealStatus::kDealStatusFundsNeeded:
            self->processPaymentRequest(deal_state);
            break;
          case DealStatus::kDealStatusOngoing:
            self->processNextResponse(deal_state);
            break;
          default:
            self->failDeal(deal_state,
                           RetrievalClientError::kUnknownResponseReceived);
        }
      }
    });
  }

  void RetrievalClientImpl::processPaymentRequest(const DealState &deal_state) {
  }

  void RetrievalClientImpl::completeDeal(const DealState &deal_state) {
    if (!deal_state.stream->stream()->isClosed()) {
      deal_state.stream->stream()->close(deal_state.handler);
    }
  }

  void RetrievalClientImpl::failDeal(const DealState &deal_state,
                                     const RetrievalClientError &error) {
    if (!deal_state.stream->stream()->isClosed()) {
      deal_state.stream->stream()->close(deal_state.handler);
    }
    deal_state.handler(error);
  }

}  // namespace fc::markets::retrieval::client
