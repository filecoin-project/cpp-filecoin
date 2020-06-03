/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/provider/query_responder/query_responder_impl.hpp"
#include "codec/cbor/cbor.hpp"

namespace fc::markets::retrieval::provider {
  void QueryResponderImpl::onNewRequest(const CborStreamShPtr &stream) {
    stream->read<QueryRequest>([self = shared_from_this(), stream](
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
      stream->write(
          response,
          [self = self->shared_from_this(), stream](outcome::result<size_t> result) {
            if (!result.has_value()) {
              self->logger_->debug("Failed to send response");
            }
            self->closeNetworkStream(stream->stream());
          });
    });
  }

  QueryItemStatus QueryResponderImpl::getItemStatus(
      const CID &payload_cid, const CID &piece_cid) const {
    auto payload_info = piece_storage_->getPayloadLocation(payload_cid);
    if (payload_info.has_value()) {
      if (payload_cid != piece_cid
          && payload_info.value().parent_piece != piece_cid) {
        return QueryItemStatus::QueryItemUnavailable;
      }
      auto piece_info =
          piece_storage_->getPieceInfo(payload_info.value().parent_piece);
      if (piece_info.has_value()) {
        return QueryItemStatus::QueryItemAvailable;
      }
    }
    return QueryItemStatus::QueryItemUnknown;
  }

  void QueryResponderImpl::closeNetworkStream(StreamShPtr stream) const {
    stream->close([self = shared_from_this()](outcome::result<void> result) {
      if (!result.has_value()) {
        self->logger_->debug("Failed to close stream");
      }
    });
  }
}  // namespace fc::markets::retrieval::provider
