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
#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "markets/retrieval/provider/retrieval_provider.hpp"
#include "storage/ipld/traverser.hpp"
#include "storage/piece/piece_storage.hpp"

namespace fc::markets::retrieval::provider {
  using common::libp2p::CborHost;
  using common::libp2p::CborStream;
  using ::fc::storage::ipld::traverser::Traverser;
  using ::fc::storage::piece::PieceInfo;
  using ::fc::storage::piece::PieceStorage;
  using libp2p::Host;
  using primitives::BigInt;

  /**
   * @struct Provider config
   */
  struct ProviderConfig {
    TokenAmount price_per_byte;
    uint64_t payment_interval;
    uint64_t interval_increase;
  };

  struct DealState {
    DealState(std::shared_ptr<Ipld> ipld,
              DealProposal proposal,
              std::shared_ptr<CborStream> stream)
        : proposal{proposal},
          stream{std::move(stream)},
          current_interval{proposal.params.payment_interval},
          traverser{*ipld, proposal.payload_cid, proposal.params.selector} {}

    DealProposal proposal;
    std::shared_ptr<CborStream> stream;
    BigInt current_interval;
    BigInt total_sent;
    TokenAmount funds_received;
    TokenAmount payment_owed;
    Traverser traverser;
  };

  class RetrievalProviderImpl
      : public RetrievalProvider,
        public std::enable_shared_from_this<RetrievalProviderImpl> {
   public:
    RetrievalProviderImpl(std::shared_ptr<Host> host,
                          std::shared_ptr<api::Api> api,
                          std::shared_ptr<PieceStorage> piece_storage,
                          IpldPtr ipld,
                          const ProviderConfig &config);

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

    /**
     * Look for piece by cid and make response
     * @param query with piece look for
     * @return network response
     */
    outcome::result<QueryResponse> makeQueryResponse(const QueryRequest &query);

    /**
     * Send error response for query and close stream
     * @param stream to send and close
     * @param message error to send
     */
    void respondErrorQueryResponse(const std::shared_ptr<CborStream> &stream,
                                   const std::string &message);

    /**
     * Handle deal stream
     * @param stream - cbor stream
     */
    void handleRetrievalDeal(const std::shared_ptr<CborStream> &stream);

    /**
     * Read deal proposal and validate
     * @param deal_state
     * @return
     */
    outcome::result<void> receiveDeal(
        const std::shared_ptr<DealState> &deal_state);

    /**
     * Run decision logic
     * @param deal_state
     */
    void decideOnDeal(const std::shared_ptr<DealState> &deal_state);

    /**
     * Prepares single block for deal
     * @param deal state
     * @return block in retrieval market response format
     */
    outcome::result<DealResponse::Block> prepareNextBlock(
        const std::shared_ptr<DealState> &deal_state);

    /**
     * Prepare blocks to send (up to size of deal interval) and requests next
     * payment (owed)
     * @param deal_state
     */
    void prepareBlocks(const std::shared_ptr<DealState> &deal_state);

    /**
     * Sends response - possibly payload blocks and payment request
     * @param deal_state
     * @param response
     */
    void sendRetrievalResponse(const std::shared_ptr<DealState> &deal_state,
                               const DealResponse &response);

    void processPayment(const std::shared_ptr<DealState> &deal_state,
                        const DealStatus &payment_status);

    /**
     * Send error response for retrieval proposal and close stream
     * @param stream to communicate through and close
     * @param status - response deal status
     * @param message - error description
     */
    void respondErrorRetrievalDeal(const std::shared_ptr<CborStream> &stream,
                                   const DealStatus &status,
                                   const std::string &message);

    /**
     * Finalize deal - send deal complete response and close stream
     * @param deal_state
     */
    void finalizeDeal(const std::shared_ptr<DealState> &deal_state);

    std::shared_ptr<CborHost> host_;
    std::shared_ptr<api::Api> api_;
    Address miner_address;
    std::shared_ptr<PieceStorage> piece_storage_;
    std::shared_ptr<Ipld> ipld_;
    ProviderConfig config_;
    common::Logger logger_ = common::createLogger("RetrievalProvider");
  };
}  // namespace fc::markets::retrieval::provider

#endif
