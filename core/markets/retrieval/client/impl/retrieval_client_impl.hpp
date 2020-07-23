/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_CLIENT_IMPL_HPP

#include <memory>

#include <libp2p/host/host.hpp>
#include "api/api.hpp"
#include "common/logger.hpp"
#include "markets/retrieval/client/retrieval_client.hpp"
#include "markets/retrieval/client/retrieval_client_error.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/ipld/verifier.hpp"
#include "vm/actor/builtin/payment_channel/payment_channel_actor_state.hpp"

namespace fc::markets::retrieval::client {
  using api::Api;
  using common::libp2p::CborHost;
  using common::libp2p::CborStream;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::ipld::verifier::Verifier;
  using libp2p::Host;
  using primitives::BigInt;
  using vm::actor::builtin::payment_channel::LaneId;

  /**
   * State of ongoing retrieval deal.
   */
  struct DealState {
    DealState(const DealProposal &proposal,
              std::shared_ptr<CborStream> stream,
              const RetrieveResponseHandler &handler,
              const Address &client_wallet,
              const Address &miner_wallet,
              const TokenAmount &total_funds)
        : proposal{proposal},
          stream{std::move(stream)},
          handler{handler},
          client_wallet{client_wallet},
          miner_wallet{miner_wallet},
          total_funds(total_funds),
          current_interval{proposal.params.payment_interval},
          deal_status{DealStatus::kDealStatusOngoing},
          verifier{proposal.payload_cid, proposal.params.selector} {}

    DealProposal proposal;
    std::shared_ptr<CborStream> stream;
    RetrieveResponseHandler handler;
    Address client_wallet;
    Address miner_wallet;

    /** total cost of deal  */
    TokenAmount total_funds;

    /** Number of bytes to process in the current round */
    BigInt current_interval;

    /** Bytes have been paid */
    BigInt bytes_paid_for;

    /** Total bytes received */
    BigInt total_received;

    /** Payment channel actor address */
    Address payment_channel_address;

    /** Payment channel lane */
    LaneId lane_id;

    /** Funds already have been paid to miner during deal */
    TokenAmount funds_spent;

    /**
     * Status of current deal.
     * Actually statuses used:
     * - kDealStatusOngoing - deal in active state
     * - kDealStatusBlocksComplete - blocks sending complete, finalize deal
     */
    DealStatus deal_status;

    /**
     * Received ipld blocks verifier
     */
    Verifier verifier;
  };

  class RetrievalClientImpl
      : public RetrievalClient,
        public std::enable_shared_from_this<RetrievalClientImpl> {
   public:
    /**
     * @brief Constructor
     * @param host - libp2p network backend
     * @param IpfsDatastore - ipfs datastore
     */
    RetrievalClientImpl(std::shared_ptr<Host> host,
                        std::shared_ptr<Api> api,
                        std::shared_ptr<IpfsDatastore> ipfs);

    outcome::result<std::vector<PeerInfo>> findProviders(
        const CID &piece_cid) const override;

    void query(const PeerInfo &peer,
               const QueryRequest &request,
               const QueryResponseHandler &response_handler) override;

    void retrieve(const CID &payload_cid,
                  const DealProposalParams &deal_params,
                  const PeerInfo &provider_peer,
                  const Address &client_wallet,
                  const Address &miner_wallet,
                  const TokenAmount &total_funds,
                  const RetrieveResponseHandler &handler) override;

   private:
    void closeQueryStream(const std::shared_ptr<CborStream> &stream,
                          const QueryResponseHandler &handler);

    void proposeDeal(const std::shared_ptr<DealState> &deal_state);

    /**
     * Funds payment channel actor if exists or create if doesn't
     * @param deal_state
     * @return error if encountered
     */
    outcome::result<void> createAndFundPaymentChannel(
        const std::shared_ptr<DealState> &deal_state);

    /**
     * Process one block from response
     * @param deal_state - state of ongoing deal
     * @param block to process
     * @return true if last block processed (blocks are completed)
     */
    outcome::result<bool> processBlock(
        const std::shared_ptr<DealState> &deal_state,
        const DealResponse::Block &block);

    void setupPaymentChannelStart(const std::shared_ptr<DealState> &deal_state);

    void processNextResponse(const std::shared_ptr<DealState> &deal_state);

    void processPaymentRequest(const std::shared_ptr<DealState> &deal_state,
                               const TokenAmount &payment_requested,
                               bool last_payment);

    /**
     * Read and process last response with provider deal status
     * @param deal_state
     */
    void finalizeDeal(const std::shared_ptr<DealState> &deal_state);

    void completeDeal(const std::shared_ptr<DealState> &deal_state);

    void failDeal(const std::shared_ptr<DealState> &deal_state,
                  const std::error_code &error);

    DealId next_deal_id;
    std::shared_ptr<CborHost> host_;
    std::shared_ptr<Api> api_;
    std::shared_ptr<IpfsDatastore> ipfs_;
    common::Logger logger_ = common::createLogger("RetrievalMarketClient");
  };

}  // namespace fc::markets::retrieval::client

#endif
