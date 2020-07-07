/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/provider/impl/retrieval_provider_impl.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "markets/common.hpp"
#include "storage/piece/impl/piece_storage_error.hpp"

#define SELF_IF_ERROR_RESPOND_AND_RETURN(result, status, stream)               \
  if (result.has_error()) {                                                    \
    self->logger_->error(result.error().message());                            \
    self->respondErrorRetrievalDeal(stream, status, result.error().message()); \
    return;                                                                    \
  }

namespace fc::markets::retrieval::provider {
  using ::fc::storage::piece::PieceStorageError;

  RetrievalProviderImpl::RetrievalProviderImpl(
      std::shared_ptr<Host> host,
      std::shared_ptr<api::Api> api,
      std::shared_ptr<PieceStorage> piece_storage)
      : host_{std::make_shared<CborHost>(host)},
        api_{std::move(api)},
        piece_storage_{std::move(piece_storage)},
        config_{} {}

  void RetrievalProviderImpl::start() {
    host_->setCborProtocolHandler(
        kQueryProtocolId,
        [self{shared_from_this()}](auto stream) { self->handleQuery(stream); });
    host_->setCborProtocolHandler(kRetrievalProtocolId,
                                  [self{shared_from_this()}](auto stream) {
                                    self->handleRetrievalDeal(stream);
                                  });
    logger_->info("has been launched with ID "
                  + peerInfoToPrettyString(host_->getPeerInfo()));
  }

  void RetrievalProviderImpl::setPricePerByte(TokenAmount amount) {
    config_.price_per_byte = amount;
  }

  void RetrievalProviderImpl::setPaymentInterval(
      uint64_t payment_interval, uint64_t payment_interval_increase) {
    config_.payment_interval = payment_interval;
    config_.interval_increase = payment_interval_increase;
  }

  void RetrievalProviderImpl::handleQuery(
      const std::shared_ptr<CborStream> &stream) {
    stream->read<QueryRequest>([self{shared_from_this()},
                                stream](auto request_res) {
      if (request_res.has_error()) {
        self->respondErrorQueryResponse(stream, request_res.error().message());
        return;
      }
      auto response_res = self->makeQueryResponse(request_res.value());
      if (response_res.has_error()) {
        self->respondErrorQueryResponse(stream, response_res.error().message());
        return;
      }
      stream->write(response_res.value(), [self, stream](auto written) {
        if (written.has_error()) {
          self->logger_->error("Error while error response "
                               + written.error().message());
        }
        closeStreamGracefully(stream, self->logger_);
      });
    });
  }

  outcome::result<QueryResponse> RetrievalProviderImpl::makeQueryResponse(
      const QueryRequest &query) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(miner_worker_address,
                api_->StateMinerWorker(miner_address, tipset_key));

    OUTCOME_TRY(piece_available,
                piece_storage_->hasPieceInfo(query.payload_cid,
                                             query.params.piece_cid));
    if (!piece_available) {
      return QueryResponse{
          .response_status = QueryResponseStatus::kQueryResponseUnavailable,
          .item_status = QueryItemStatus::kQueryItemUnavailable};
    }
    OUTCOME_TRY(piece_size,
                piece_storage_->getPieceSize(query.payload_cid,
                                             query.params.piece_cid));
    return QueryResponse{
        .response_status = QueryResponseStatus::kQueryResponseAvailable,
        .item_status = QueryItemStatus::kQueryItemAvailable,
        .item_size = piece_size,
        .payment_address = miner_worker_address,
        .min_price_per_byte = config_.price_per_byte,
        .payment_interval = config_.payment_interval,
        .interval_increase = config_.interval_increase};
  }

  void RetrievalProviderImpl::respondErrorQueryResponse(
      const std::shared_ptr<CborStream> &stream, const std::string &message) {
    QueryResponse response;
    response.response_status = QueryResponseStatus::kQueryResponseError;
    response.item_status = QueryItemStatus::kQueryItemUnknown;
    response.message = message;
    stream->write(response, [self{shared_from_this()}, stream](auto written) {
      if (written.has_error()) {
        self->logger_->error("Error while error response "
                             + written.error().message());
      }
      closeStreamGracefully(stream, self->logger_);
    });
  }

  void RetrievalProviderImpl::handleRetrievalDeal(
      const std::shared_ptr<CborStream> &stream) {
    stream->read<DealProposal>(
        [self{shared_from_this()}, stream](auto proposal_res) {
          SELF_IF_ERROR_RESPOND_AND_RETURN(
              proposal_res, DealStatus::kDealStatusErrored, stream);
          auto deal_state =
              std::make_shared<DealState>(proposal_res.value(), stream);
          SELF_IF_ERROR_RESPOND_AND_RETURN(self->receiveDeal(deal_state),
                                           DealStatus::kDealStatusErrored,
                                           stream);
        });
  }

  outcome::result<void> RetrievalProviderImpl::receiveDeal(
      const std::shared_ptr<DealState> &deal_state) {
    OUTCOME_TRY(piece_available,
                piece_storage_->hasPieceInfo(deal_state->proposal.payload_cid,
                                             boost::none));
    if (!piece_available) {
      respondErrorRetrievalDeal(deal_state->stream,
                                DealStatus::kDealStatusFailed,
                                "Payload not found");
      return outcome::success();
    }

    if (deal_state->proposal.params.price_per_byte < config_.price_per_byte
        || deal_state->proposal.params.payment_interval
               > config_.payment_interval
        || deal_state->proposal.params.payment_interval_increase
               > config_.interval_increase) {
      respondErrorRetrievalDeal(deal_state->stream,
                                DealStatus::kDealStatusRejected,
                                "Deal parameters not accepted");
      return outcome::success();
    }

    decideOnDeal(deal_state);

    return outcome::success();
  }

  void RetrievalProviderImpl::decideOnDeal(
      const std::shared_ptr<DealState> &deal_state) {
    // TODO (a.chernyshov) run decision logic
    bool deal_accepted = true;
    if (!deal_accepted) {
      respondErrorRetrievalDeal(deal_state->stream,
                                DealStatus::kDealStatusRejected,
                                "Deal not accepted");
    } else {
      DealResponse response;
      response.deal_id = deal_state->proposal.deal_id;
      response.status = DealStatus::kDealStatusAccepted;
      deal_state->stream->write(
          response,
          [self{shared_from_this()},
           deal_state{std::move(deal_state)}](auto written) {
            SELF_IF_ERROR_RESPOND_AND_RETURN(
                written, DealStatus::kDealStatusErrored, deal_state->stream);
            self->prepareBlocks(deal_state);
          });
    }
  }

  void RetrievalProviderImpl::prepareBlocks(
      const std::shared_ptr<DealState> &deal_state) {
    TokenAmount total_sent = deal_state->total_sent;
    auto total_paid_for =
        deal_state->funds_received / deal_state->proposal.params.price_per_byte;

    while (total_sent - total_paid_for < deal_state->current_interval) {
      // TODO add blocks
      // total_sent += block.length
    }

    // TODO check if completed
    bool completed = true;
    DealStatus payment_status;
    if (completed) {
      payment_status = DealStatus::kDealStatusFundsNeededLastPayment;
    } else {
      payment_status = DealStatus::kDealStatusFundsNeeded;
    }

    auto payment_owed = (total_sent - total_paid_for)
                        * deal_state->proposal.params.price_per_byte;

    DealResponse response;
    response.deal_id = deal_state->proposal.deal_id;
    response.status = payment_status;
    response.payment_owed = payment_owed;
    // TODO add response.blocks
    sendRetrievalResponse(deal_state, response);
  }

  void RetrievalProviderImpl::sendRetrievalResponse(
      const std::shared_ptr<DealState> &deal_state,
      const DealResponse &response) {
    deal_state->stream->write(
        response,
        [self{shared_from_this()},
         deal_state{
             std::move(deal_state),
         },
         payment_owed{response.payment_owed},
         payment_status{response.status}](auto written) {
          SELF_IF_ERROR_RESPOND_AND_RETURN(
              written, DealStatus::kDealStatusErrored, deal_state->stream);

          // data sent, now client have to pay
          deal_state->payment_owed = payment_owed;
          // TODO update deal_state->total_sent
          self->processPayment(deal_state, payment_status);
        });
  }

  void RetrievalProviderImpl::processPayment(
      const std::shared_ptr<DealState> &deal_state,
      const DealStatus &payment_status) {
    deal_state->stream->read<DealPayment>([self{shared_from_this()},
                                           deal_state{std::move(deal_state)},
                                           payment_status](auto payment_res) {
      SELF_IF_ERROR_RESPOND_AND_RETURN(
          payment_res, DealStatus::kDealStatusErrored, deal_state->stream);

      // TODO redeem voucher

      // TODO update deal_state->funds_received

      // TODO check if received < deal_state.payment_owed
      bool partial_payment_received = false;
      if (partial_payment_received) {
        DealResponse response;
        response.deal_id = deal_state->proposal.deal_id;
        response.status = payment_status;
        // TODO response.payment_owed = payment_owed - received;
        self->sendRetrievalResponse(deal_state, response);
        return;
      }

      // last payment received => deal completed
      if (payment_status == DealStatus::kDealStatusFundsNeededLastPayment) {
        self->finalizeDeal(deal_state);
      } else {
        deal_state->current_interval +=
            deal_state->proposal.params.payment_interval_increase;
        self->prepareBlocks(deal_state);
      }
    });
  }

  void RetrievalProviderImpl::respondErrorRetrievalDeal(
      const std::shared_ptr<CborStream> &stream,
      const DealStatus &status,
      const std::string &message) {
    DealResponse response;
    response.status = status;
    response.message = message;

    stream->write(response, [self{shared_from_this()}, stream](auto written) {
      if (written.has_error()) {
        self->logger_->error("Error while error response "
                             + written.error().message());
      }
      closeStreamGracefully(stream, self->logger_);
    });
  }

  void RetrievalProviderImpl::finalizeDeal(
      const std::shared_ptr<DealState> &deal_state) {
    DealResponse response;
    response.deal_id = deal_state->proposal.deal_id;
    response.status = DealStatus::kDealStatusCompleted;
    deal_state->stream->write(
        response,
        [self{shared_from_this()},
         deal_state{std::move(deal_state)}](auto written) {
          SELF_IF_ERROR_RESPOND_AND_RETURN(
              written, DealStatus::kDealStatusErrored, deal_state->stream);
          closeStreamGracefully(deal_state->stream, self->logger_);
        });
  }

}  // namespace fc::markets::retrieval::provider
