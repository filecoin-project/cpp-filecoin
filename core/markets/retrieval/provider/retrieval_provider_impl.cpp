/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <thread>

#include "common/libp2p/cbor_stream.hpp"
#include "markets/retrieval/provider/query_responder/query_responder_impl.hpp"
#include "markets/retrieval/provider/retrieval_provider_impl.hpp"

namespace fc::markets::retrieval::provider {
  using common::libp2p::CborStream;

  void RetrievalProviderImpl::start() {
    host_service_->setProtocolHandler(
        kQueryProtocolId, [self = shared_from_this()](StreamShPtr stream) {
          self->query_responder_->onNewRequest(
              std::make_shared<CborStream>(stream));
        });
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
