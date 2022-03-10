/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <libp2p/host/host.hpp>
#include "api/full_node/node_api.hpp"
#include "common/libp2p/cbor_stream.hpp"
#include "common/logger.hpp"
#include "data_transfer/dt.hpp"
#include "markets/retrieval/client/retrieval_client.hpp"
#include "markets/retrieval/client/retrieval_client_error.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/ipld/traverser.hpp"
#include "vm/actor/builtin/types/payment_channel/voucher.hpp"

namespace fc::markets::retrieval::client {
  using api::FullNodeApi;
  using common::libp2p::CborStream;
  using data_transfer::DataTransfer;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::ipld::traverser::Traverser;
  using libp2p::Host;
  using primitives::BigInt;
  using vm::actor::builtin::types::payment_channel::LaneId;

  /**
   * State of ongoing retrieval deal.
   */
  struct DealState : public RetrievalDeal {
    DealState(const DealProposalV1_0_0 &proposal,
              const IpldPtr &ipld,
              RetrieveResponseHandler handler,
              Address client_wallet,
              Address miner_wallet,
              TokenAmount total_funds);

    /**
     * Received ipld blocks verifier
     */
    Traverser traverser;
    RetrieveResponseHandler handler;

    mutable std::mutex pending_mutex;
    std::optional<std::vector<DealResponse>> pending;
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
                        std::shared_ptr<DataTransfer> datatransfer,
                        std::shared_ptr<FullNodeApi> api,
                        std::shared_ptr<IpfsDatastore> ipfs);

    outcome::result<std::vector<PeerInfo>> findProviders(
        const CID &piece_cid) const override;

    void query(const RetrievalPeer &provider_peer,
               const QueryRequest &request,
               const QueryResponseHandler &cb) override;

    outcome::result<void> retrieve(
        const CID &payload_cid,
        const DealProposalParams &deal_params,
        const TokenAmount &total_funds,
        const RetrievalPeer &provider_peer,
        const Address &client_wallet,
        const Address &miner_wallet,
        const RetrieveResponseHandler &handler) override;

   private:
    /**
     * Callback for datatransfer pull on data
     */
    void onPullData(const std::shared_ptr<DealState> &deal,
                    const std::string &type,
                    BytesIn voucher);

    /**
     * Callback for datatransfer pull on cid
     */
    void onPullCid(const std::shared_ptr<DealState> &deal, const CID &);

    /**
     * Get libp2p PeerInfo with multiaddresses
     * @param provider_peer
     */
    outcome::result<PeerInfo> getPeerInfo(const RetrievalPeer &provider_peer);

    void processPaymentRequest(const std::shared_ptr<DealState> &deal_state,
                               const TokenAmount &payment_requested);

    void failDeal(const std::shared_ptr<DealState> &deal_state,
                  const std::error_code &error);

    outcome::result<std::vector<std::shared_ptr<RetrievalDeal>>> getRetrievals()
        override;

    DealId next_deal_id{};
    std::shared_ptr<Host> host_;
    std::shared_ptr<DataTransfer> datatransfer_;
    std::shared_ptr<FullNodeApi> api_;
    std::shared_ptr<IpfsDatastore> ipfs_;
    std::vector<std::shared_ptr<DealState>> deals;
    common::Logger logger_ = common::createLogger("RetrievalMarketClient");
  };

}  // namespace fc::markets::retrieval::client
