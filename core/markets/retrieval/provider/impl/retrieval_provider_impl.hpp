/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_IMPL_HPP

#include "api/api.hpp"
#include "common/libp2p/cbor_host.hpp"
#include "common/libp2p/cbor_stream.hpp"
#include "common/logger.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"
#include "markets/retrieval/provider/retrieval_provider.hpp"
#include "storage/piece/piece_storage.hpp"

namespace fc::markets::retrieval::provider {
  using common::libp2p::CborHost;
  using common::libp2p::CborStream;
  using ::fc::storage::piece::PieceInfo;
  using ::fc::storage::piece::PieceStorage;
  using libp2p::Host;

  /**
   * @struct Provider config
   */
  struct ProviderConfig {
    TokenAmount price_per_byte;
    uint64_t payment_interval;
    uint64_t interval_increase;
  };

  class RetrievalProviderImpl
      : public RetrievalProvider,
        public std::enable_shared_from_this<RetrievalProviderImpl> {
   public:
    RetrievalProviderImpl(std::shared_ptr<Host> host,
                          std::shared_ptr<api::Api> api,
                          std::shared_ptr<PieceStorage> piece_storage);

    void start() override;

    void setPricePerByte(TokenAmount amount) override;

    void setPaymentInterval(uint64_t payment_interval,
                            uint64_t payment_interval_increase) override;

   private:
    /**
     * Handle query stream
     * @param stream - cbor stream
     */
    void handleQuery(const std::shared_ptr<CborStream> &stream);

    outcome::result<QueryResponse> makeQueryResponse(const QueryRequest &query);
    void respondErrorQueryResponse(const std::shared_ptr<CborStream> &stream,
                                   const std::string &message);

    std::shared_ptr<CborHost> host_;
    std::shared_ptr<api::Api> api_;
    Address miner_address;
    std::shared_ptr<PieceStorage> piece_storage_;
    ProviderConfig config_;
    common::Logger logger_ = common::createLogger("RetrievalProvider");
  };
}  // namespace fc::markets::retrieval::provider

#endif
