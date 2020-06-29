/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_IMPL_HPP

#include "api/api.hpp"
#include "common/logger.hpp"
#include "markets/pieceio/pieceio.hpp"
#include "markets/retrieval/provider/query_responder/query_responder_impl.hpp"
#include "markets/retrieval/provider/retrieval_handler/retrieval_handler_impl.hpp"
#include "markets/retrieval/provider/retrieval_provider_types.hpp"
#include "markets/retrieval/retrieval_provider.hpp"

namespace fc::markets::retrieval::provider {
  class RetrievalProviderImpl
      : public RetrievalProvider,
        public std::enable_shared_from_this<RetrievalProviderImpl> {
   protected:
    using HostServiceShPtr = std::shared_ptr<HostService>;
    using PieceIOShPtr = std::shared_ptr<fc::markets::pieceio::PieceIO>;
    using ApiShPtr = std::shared_ptr<api::Api>;
    using StreamShPtr = std::shared_ptr<libp2p::connection::Stream>;
    using QueryResponderShPtr = std::shared_ptr<QueryResponderImpl>;
    using RetrievalHandlerShPtr = std::shared_ptr<RetrievalHandlerImpl>;

   public:
    RetrievalProviderImpl(HostServiceShPtr host_service,
                          PieceIOShPtr piece_io,
                          ApiShPtr api);

    void start() override;

    void setPricePerByte(TokenAmount amount) override;

    void setPaymentInterval(uint64_t payment_interval,
                            uint64_t payment_interval_increase) override;

   private:
    HostServiceShPtr host_service_;
    common::Logger logger_;
    ProviderConfig config_;
    ApiShPtr api_;
    QueryResponderShPtr query_responder_;
    RetrievalHandlerShPtr retrieval_handler_;
  };
}  // namespace fc::markets::retrieval::provider

#endif
