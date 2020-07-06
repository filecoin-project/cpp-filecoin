/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/provider/impl/retrieval_provider_impl.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "markets/common.hpp"
#include "storage/piece/impl/piece_storage_error.hpp"

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
    // TODO move set handler to init
    host_->setCborProtocolHandler(
        kQueryProtocolId,
        [self{shared_from_this()}](auto stream) { self->handleQuery(stream); });
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

}  // namespace fc::markets::retrieval::provider
