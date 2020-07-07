/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_IMPL_HPP

#include "api/api.hpp"
#include "common/logger.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"
#include "markets/retrieval/provider/query_responder/query_responder_impl.hpp"
#include "markets/retrieval/provider/retrieval_provider.hpp"
#include "markets/retrieval/provider/retrieval_provider_types.hpp"
#include "storage/piece/piece_storage.hpp"

namespace fc::markets::retrieval::provider {
  using libp2p::Host;

  class RetrievalProviderImpl
      : public RetrievalProvider,
        public std::enable_shared_from_this<RetrievalProviderImpl> {
   protected:
    using PieceStorageShPtr =
        std::shared_ptr<::fc::storage::piece::PieceStorage>;
    using ApiShPtr = std::shared_ptr<api::Api>;
    using StreamShPtr = std::shared_ptr<libp2p::connection::Stream>;
    using QueryResponderShPtr = std::shared_ptr<QueryResponderImpl>;

   public:
    RetrievalProviderImpl(std::shared_ptr<Host> host_service,
                          PieceStorageShPtr piece_storage,
                          ApiShPtr api)
        : host_service_{std::move(host_service)},
          piece_storage_{std::move(piece_storage)},
          logger_{common::createLogger("Retrieval provider")},
          config_{},
          api_{std::move(api)},
          query_responder_{std::make_shared<QueryResponderImpl>(
              piece_storage_, api_, logger_, config_)} {}

    void start() override;

    void setPricePerByte(TokenAmount amount) override;

    void setPaymentInterval(uint64_t payment_interval,
                            uint64_t payment_interval_increase) override;

   private:
    std::shared_ptr<Host> host_service_;
    PieceStorageShPtr piece_storage_;
    common::Logger logger_;
    ProviderConfig config_;
    ApiShPtr api_;
    QueryResponderShPtr query_responder_;
  };
}  // namespace fc::markets::retrieval::provider

#endif
