/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "retrieval_client_impl.hpp"
#include "codec/cbor/cbor.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"

#define _SELF_IF_ERROR_FAIL_AND_RETURN(var, expr) \
  auto &&var = (expr);                            \
  if (var.has_error()) {                          \
    self->failDeal(deal_state, var.error());      \
    return;                                       \
  }
#define SELF_IF_ERROR_FAIL_AND_RETURN(expr) \
  _SELF_IF_ERROR_FAIL_AND_RETURN(UNIQUE_NAME(_r), expr)

namespace fc::markets::retrieval::client {

  RetrievalClientImpl::RetrievalClientImpl(std::shared_ptr<Host> host,
                                           std::shared_ptr<Api> api,
                                           std::shared_ptr<IpfsDatastore> ipfs)
      : host_{std::make_shared<CborHost>(host)},
        api_{std::move(api)},
        ipfs_{std::move(ipfs)} {}

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
                  self->closeQueryStream(stream, response_handler);
                  return;
                }
                stream->template read<QueryResponse>(
                    [self, response_handler, stream](auto response) {
                      self->closeQueryStream(stream, response_handler);
                      response_handler(response);
                    });
              });
        });
  }

  void RetrievalClientImpl::closeQueryStream(
      const std::shared_ptr<CborStream> &stream,
      const QueryResponseHandler &response_handler) {
    if (!stream->stream()->isClosed()) {
      stream->stream()->close([response_handler](auto closed) {
        if (closed.has_error()) {
          response_handler(closed.error());
        }
      });
    }
  }

  void RetrievalClientImpl::retrieve(const CID &payload_cid,
                                     const DealProposalParams &deal_params,
                                     const PeerInfo &provider_peer,
                                     const Address &client_wallet,
                                     const Address &miner_wallet,
                                     const TokenAmount &total_funds,
                                     const RetrieveResponseHandler &handler) {
    DealProposal proposal{.payload_cid = payload_cid,
                          .deal_id = next_deal_id++,
                          .params = deal_params};
    host_->newCborStream(provider_peer,
                         kRetrievalProtocolId,
                         [self{shared_from_this()},
                          proposal,
                          client_wallet,
                          miner_wallet,
                          total_funds,
                          handler](auto stream_res) {
                           if (stream_res.has_error()) {
                             handler(stream_res.error());
                             return;
                           }
                           auto deal_state =
                               std::make_shared<DealState>(proposal,
                                                           stream_res.value(),
                                                           handler,
                                                           client_wallet,
                                                           miner_wallet,
                                                           total_funds);
                           self->proposeDeal(deal_state);
                         });
  }

  void RetrievalClientImpl::proposeDeal(
      const std::shared_ptr<DealState> &deal_state) {
    deal_state->stream->write(
        deal_state->proposal,
        [self{shared_from_this()},
         deal_state{std::move(deal_state)}](auto written) {
          SELF_IF_ERROR_FAIL_AND_RETURN(written);
          deal_state->stream->template read<DealResponse>(
              [self, deal_state{std::move(deal_state)}](auto response) {
                SELF_IF_ERROR_FAIL_AND_RETURN(response);
                switch (response.value().status) {
                  case DealStatus::kDealStatusAccepted:
                    self->setupPaymentChannelStart(deal_state);
                    break;
                  case DealStatus::kDealStatusRejected:
                    self->failDeal(deal_state,
                                   RetrievalClientError::kResponseDealRejected);
                    break;
                  case DealStatus::kDealStatusDealNotFound:
                    self->failDeal(deal_state,
                                   RetrievalClientError::kResponseNotFound);
                    break;
                  default:
                    self->failDeal(
                        deal_state,
                        RetrievalClientError::kUnknownResponseReceived);
                }
              });
        });
  }

  outcome::result<void> RetrievalClientImpl::createAndFundPaymentChannel(
      const std::shared_ptr<DealState> &deal_state) {
    // no need to check again if message is committed - it is already checcked
    // in API
    OUTCOME_TRY(paychannel_funded,
                api_->PaychGet(deal_state->client_wallet,
                               deal_state->miner_wallet,
                               deal_state->total_funds));
    deal_state->payment_channel_address = paychannel_funded.channel;
    OUTCOME_TRYA(deal_state->lane_id,
                 api_->PaychAllocateLane(paychannel_funded.channel));
    return outcome::success();
  }

  outcome::result<bool> RetrievalClientImpl::processBlock(
      const std::shared_ptr<DealState> &deal_state,
      const DealResponse::Block &block) {
    // reconstruct cid from parsed prefix and calculated data multihash
    auto prefix_reader = gsl::make_span(block.prefix);
    OUTCOME_TRY(cid, CID::read(prefix_reader, true));
    if (!prefix_reader.empty()) {
      return RetrievalClientError::kBlockCidParseError;
    }
    cid.content_address =
        crypto::Hasher::calculate(cid.content_address.getType(), block.data);

    OUTCOME_TRY(completed,
                deal_state->verifier.verifyNextBlock(cid, block.data));
    OUTCOME_TRY(ipfs_->set(cid, block.data));
    deal_state->total_received += block.data.size();
    return outcome::success(completed);
  }

  void RetrievalClientImpl::setupPaymentChannelStart(
      const std::shared_ptr<DealState> &deal_state) {
    auto paychannel_funded = createAndFundPaymentChannel(deal_state);
    if (paychannel_funded.has_error()) {
      failDeal(deal_state, paychannel_funded.error());
      return;
    }
    processNextResponse(deal_state);
  }

  void RetrievalClientImpl::processNextResponse(
      const std::shared_ptr<DealState> &deal_state) {
    deal_state->stream->read<DealResponse>([self{shared_from_this()},
                                            deal_state{std::move(deal_state)}](
                                               auto response) {
      SELF_IF_ERROR_FAIL_AND_RETURN(response);

      bool completed =
          deal_state->deal_status == DealStatus::kDealStatusBlocksComplete;
      if (!completed) {
        for (auto &&block : response.value().blocks) {
          auto maybe_completed = self->processBlock(deal_state, block);
          SELF_IF_ERROR_FAIL_AND_RETURN(maybe_completed);
          if (maybe_completed.value() == true) {
            completed = true;
            break;
          }
        }
      }

      if (completed) {
        switch (response.value().status) {
          case DealStatus::kDealStatusFundsNeededLastPayment:
            self->processPaymentRequest(
                deal_state, response.value().payment_owed, true);
            break;
          case DealStatus::kDealStatusBlocksComplete:
            deal_state->deal_status = DealStatus::kDealStatusBlocksComplete;
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
            self->processPaymentRequest(
                deal_state, response.value().payment_owed, false);
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

  void RetrievalClientImpl::processPaymentRequest(
      const std::shared_ptr<DealState> &deal_state,
      const TokenAmount &payment_requested,
      bool last_payment) {
    // check that requested is not exceed total funds for deal
    if (deal_state->funds_spent + payment_requested > deal_state->total_funds) {
      failDeal(deal_state, RetrievalClientError::kRequestedTooMuch);
      return;
    }
    // not enough bytes received between payment request
    if ((deal_state->total_received - deal_state->bytes_paid_for
         < deal_state->current_interval)
        && !last_payment) {
      failDeal(deal_state,
               RetrievalClientError::kBadPaymentRequestBytesNotReceived);
      return;
    }
    // check if payment requested > price of bytes received
    if (payment_requested
        > ((deal_state->total_received - deal_state->bytes_paid_for)
           * deal_state->proposal.params.price_per_byte)) {
      failDeal(deal_state, RetrievalClientError::kBadPaymentRequestTooMuch);
      return;
    }

    auto maybe_voucher =
        api_->PaychVoucherCreate(deal_state->payment_channel_address,
                                 deal_state->funds_spent + payment_requested,
                                 deal_state->lane_id);
    if (maybe_voucher.has_error()) {
      failDeal(deal_state, maybe_voucher.error());
      return;
    }

    DealPayment deal_payment{
        .deal_id = deal_state->proposal.deal_id,
        .payment_channel = deal_state->payment_channel_address,
        .payment_voucher = maybe_voucher.value()};
    deal_state->stream->write<DealPayment>(
        deal_payment,
        [self{shared_from_this()},
         deal_state{std::move(deal_state)},
         payment_requested,
         last_payment](auto written) {
          SELF_IF_ERROR_FAIL_AND_RETURN(written);

          deal_state->funds_spent += payment_requested;
          BigInt bytes_paid_in_round = bigdiv(payment_requested, deal_state->proposal.params.price_per_byte);
          if (bytes_paid_in_round >= deal_state->current_interval) {
            deal_state->current_interval +=
                deal_state->proposal.params.payment_interval_increase;
          }
          deal_state->bytes_paid_for += bytes_paid_in_round;

          if (last_payment) {
            self->finalizeDeal(deal_state);
          } else {
            self->processNextResponse(deal_state);
          }
        });
  }

  void RetrievalClientImpl::finalizeDeal(
      const std::shared_ptr<DealState> &deal_state) {
    deal_state->stream->read<DealResponse>(
        [self{shared_from_this()},
         deal_state{std::move(deal_state)}](auto response) {
          SELF_IF_ERROR_FAIL_AND_RETURN(response);
          if (response.value().status != DealStatus::kDealStatusCompleted) {
            self->failDeal(deal_state,
                           RetrievalClientError::kUnknownResponseReceived);
            return;
          }
          self->completeDeal(deal_state);
        });
  }

  void RetrievalClientImpl::completeDeal(
      const std::shared_ptr<DealState> &deal_state) {
    if (!deal_state->stream->stream()->isClosed()) {
      deal_state->stream->stream()->close(deal_state->handler);
    }
  }

  void RetrievalClientImpl::failDeal(
      const std::shared_ptr<DealState> &deal_state,
      const std::error_code &error) {
    if (!deal_state->stream->stream()->isClosed()) {
      deal_state->stream->stream()->close(deal_state->handler);
    }
    deal_state->handler(error);
  }

}  // namespace fc::markets::retrieval::client
