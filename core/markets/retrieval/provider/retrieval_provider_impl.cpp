/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <thread>

#include "common/libp2p/cbor_stream.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"
#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "markets/retrieval/provider/retrieval_provider_impl.hpp"

namespace fc::markets::retrieval::provider {
  using common::libp2p::CborStream;

  RetrievalProviderImpl::RetrievalProviderImpl(HostServiceShPtr host_service,
                                               PieceIOShPtr piece_io,
                                               ApiShPtr api)
      : host_service_{std::move(host_service)},
        logger_{common::createLogger("Retrieval provider")},
        config_{},
        api_{std::move(api)},
        query_responder_{std::make_shared<QueryResponderImpl>(
            piece_io, api_, logger_, config_)},
        retrieval_handler_{std::make_shared<RetrievalHandlerImpl>(piece_io)} {}

  void RetrievalProviderImpl::start() {
    host_service_->setProtocolHandler(
        kQueryProtocolId, [self = shared_from_this()](StreamShPtr stream) {
          self->query_responder_->onNewRequest(
              std::make_shared<CborStream>(stream));
        });
    host_service_->setProtocolHandler(
        kRetrievalProtocolId,
        [self = shared_from_this()](StreamShPtr stream) {});
    logger_->info("has been launched with ID "
                  + host_service_->getId().toBase58());
  }

  void RetrievalProviderImpl::setPricePerByte(TokenAmount amount) {
    config_.price_per_byte = amount;
  }

  void RetrievalProviderImpl::setPaymentInterval(
      uint64_t payment_interval, uint64_t payment_interval_increase) {
    config_.payment_interval = payment_interval;
    config_.interval_increase = payment_interval_increase;
  }

}  // namespace fc::markets::retrieval::provider
