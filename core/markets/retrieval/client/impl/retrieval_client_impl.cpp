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
                                           std::shared_ptr<Dt> datatransfer,
                                           std::shared_ptr<Api> api,
                                           std::shared_ptr<IpfsDatastore> ipfs)
      : host_{std::move(host)},
        datatransfer_{std::move(datatransfer)},
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
    host_->newStream(
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
          auto stream{std::make_shared<CborStream>(stream_res.value())};
          stream->write(
              request, [self, stream, response_handler](auto written_res) {
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
    DealProposal::Named proposal{{.payload_cid = payload_cid,
                                  .deal_id = next_deal_id++,
                                  .params = deal_params}};
    auto deal{std::make_shared<DealState>(
        proposal, handler, client_wallet, miner_wallet, total_funds)};
    deal->pdtid = datatransfer_->pull(
        provider_peer,
        payload_cid,
        deal_params.selector,
        DealProposal::Named::type,
        codec::cbor::encode(proposal).value(),
        [this, deal](auto &type, auto _voucher) {
          OUTCOME_EXCEPT(res,
                         codec::cbor::decode<DealResponse::Named>(_voucher));
          if (!deal->accepted) {
            auto unseal{res.status == DealStatus::kDealStatusFundsNeededUnseal};
            deal->accepted =
                unseal || res.status == DealStatus::kDealStatusAccepted;
            if (!deal->accepted) {
              datatransfer_->pulling_out.erase(deal->pdtid);
              return deal->handler(
                  res.status == DealStatus::kDealStatusRejected
                      ? RetrievalClientError::kResponseDealRejected
                      : res.status == DealStatus::kDealStatusDealNotFound
                            ? RetrievalClientError::kResponseNotFound
                            : RetrievalClientError::kUnknownResponseReceived);
            }
            auto _paych{api_->PaychGet(
                deal->client_wallet, deal->miner_wallet, deal->total_funds)};
            if (!_paych) {
              return deal->handler(_paych.error());
            }
            auto &paych{_paych.value().channel};
            deal->payment_channel_address = paych;
            auto _lane{api_->PaychAllocateLane(paych)};
            if (!_lane) {
              return deal->handler(_lane.error());
            }
            deal->lane_id = _lane.value();
          }
          if (res.status == DealStatus::kDealStatusCompleted) {
            datatransfer_->pulling_out.erase(deal->pdtid);
            return deal->handler(outcome::success());
          }
          if (res.payment_owed) {
            processPaymentRequest(deal, deal->state.owed);
          }
        },
        [this, deal](auto &cid) {
          if (auto _data{ipfs_->get(cid)}) {
            if (auto _done{
                    deal->verifier.verifyNextBlock(cid, _data.value())}) {
              deal->state.block(_data.value().size());
              if (_done.value()) {
                deal->all_blocks = true;
                deal->state.last();
              }
              return;
            }
          }
        });
  }

  void RetrievalClientImpl::processPaymentRequest(
      const std::shared_ptr<DealState> &deal_state,
      const TokenAmount &payment_requested) {
    // check that requested is not exceed total funds for deal
    if (deal_state->state.paid + payment_requested > deal_state->total_funds) {
      failDeal(deal_state, RetrievalClientError::kRequestedTooMuch);
      return;
    }
    // not enough bytes received between payment request
    if (payment_requested != deal_state->state.owed) {
      failDeal(deal_state,
               RetrievalClientError::kBadPaymentRequestBytesNotReceived);
      return;
    }

    auto maybe_voucher =
        api_->PaychVoucherCreate(deal_state->payment_channel_address,
                                 deal_state->state.paid + payment_requested,
                                 deal_state->lane_id);
    if (maybe_voucher.has_error()) {
      failDeal(deal_state, maybe_voucher.error());
      return;
    }
    datatransfer_->pullOut(
        deal_state->pdtid,
        DealPayment::Named::type,
        codec::cbor::encode(
            DealPayment::Named{{deal_state->proposal.deal_id,
                                deal_state->payment_channel_address,
                                maybe_voucher.value()}})
            .value());
    deal_state->state.pay(payment_requested);
  }

  void RetrievalClientImpl::failDeal(
      const std::shared_ptr<DealState> &deal_state,
      const std::error_code &error) {
    datatransfer_->pulling_out.erase(deal_state->pdtid);
    deal_state->handler(error);
  }

}  // namespace fc::markets::retrieval::client
