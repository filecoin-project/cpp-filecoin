/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/provider/impl/retrieval_provider_impl.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "markets/retrieval/provider/query_responder/query_responder_impl.hpp"

namespace fc::markets::retrieval::provider {

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
    logger_->info("has been launched with ID " + peerInfoToPrettyString(host_->getPeerInfo());
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
    stream->read<QueryRequest>([self{shared_from_this()}, stream](
                                   outcome::result<QueryRequest> request_res) {
      auto payment_address_res = self->api_->WalletDefaultAddress();
      if (!payment_address_res.has_value()) {
        self->logger_->error("Failed to determine payment address");
        self->closeNetworkStream(stream->stream());
        return;
      }
      if (!request_res.has_value()) {
        self->logger_->debug("Received incorrect request");
        self->closeNetworkStream(stream->stream());
        return;
      }
      QueryResponse response;
      response.response_status = QueryResponseStatus::QueryResponseAvailable,
      response.item_status =
          self->getItemStatus(request_res.value().payload_cid,
                              request_res.value().params.piece_cid);
      response.payment_address = payment_address_res.value();
      response.min_price_per_byte = self->provider_config_.price_per_byte;
      response.payment_interval = self->provider_config_.payment_interval;
      response.interval_increase = self->provider_config_.interval_increase;
      stream->write(response,
                    [self = self->shared_from_this(),
                     stream](outcome::result<size_t> result) {
                      if (!result.has_value()) {
                        self->logger_->debug("Failed to send response");
                      }
                      self->closeNetworkStream(stream->stream());
                    });
    });
  }

}  // namespace fc::markets::retrieval::provider
