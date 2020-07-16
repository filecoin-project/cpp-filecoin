/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP

#include <libp2p/host/host.hpp>
#include <mutex>
#include "api/api.hpp"
#include "common/libp2p/cbor_host.hpp"
#include "common/logger.hpp"
#include "data_transfer/manager.hpp"
#include "fsm/fsm.hpp"
#include "host/context/host_context.hpp"
#include "markets/common.hpp"
#include "markets/discovery/discovery.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/client/client_events.hpp"
#include "markets/storage/client/storage_market_client.hpp"
#include "storage/filestore/filestore.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::markets::storage::client {

  using api::Api;
  using common::Buffer;
  using common::libp2p::CborHost;
  using common::libp2p::CborStream;
  using discovery::Discovery;
  using fc::storage::filestore::FileStore;
  using fc::storage::ipfs::IpfsDatastore;
  using fsm::FSM;
  using libp2p::Host;
  using pieceio::PieceIO;
  using ClientTransition =
      fsm::Transition<ClientEvent, StorageDealStatus, ClientDeal>;
  using ClientFSM = fsm::FSM<ClientEvent, StorageDealStatus, ClientDeal>;
  using Datastore = fc::storage::face::PersistentMap<Buffer, Buffer>;
  using Ticks = libp2p::protocol::Scheduler::Ticks;
  using DataTransfer = data_transfer::Manager;

  class StorageMarketClientImpl
      : public StorageMarketClient,
        public std::enable_shared_from_this<StorageMarketClientImpl> {
   public:
    StorageMarketClientImpl(std::shared_ptr<Host> host,
                            std::shared_ptr<boost::asio::io_context> context,
                            std::shared_ptr<Datastore> datastore,
                            std::shared_ptr<Api> api,
                            std::shared_ptr<PieceIO> piece_io);

    outcome::result<void> init() override;

    void run() override;

    outcome::result<void> stop() override;

    outcome::result<std::vector<StorageProviderInfo>> listProviders()
        const override;

    outcome::result<std::vector<StorageDeal>> listDeals(
        const Address &address) const override;

    outcome::result<std::vector<ClientDeal>> listLocalDeals() const override;

    outcome::result<ClientDeal> getLocalDeal(
        const CID &proposal_cid) const override;

    void getAsk(const StorageProviderInfo &info,
                const SignedAskHandler &signed_ask_handler) override;

    outcome::result<CID> proposeStorageDeal(
        const Address &client_address,
        const StorageProviderInfo &provider_info,
        const DataRef &data_ref,
        const ChainEpoch &start_epoch,
        const ChainEpoch &end_epoch,
        const TokenAmount &price,
        const TokenAmount &collateral,
        const RegisteredProof &registered_proof) override;

    outcome::result<StorageParticipantBalance> getPaymentEscrow(
        const Address &address) const override;

    outcome::result<void> addPaymentEscrow(const Address &address,
                                           const TokenAmount &amount) override;

   private:
    outcome::result<SignedStorageAsk> validateAskResponse(
        const outcome::result<AskResponse> &response,
        const StorageProviderInfo &info) const;

    outcome::result<std::pair<CID, UnpaddedPieceSize>> calculateCommP(
        const RegisteredProof &registered_proof, const DataRef &data_ref) const;

    outcome::result<ClientDealProposal> signProposal(
        const Address &address, const DealProposal &proposal) const;

    /**
     * Ensure client has enough funds. In case of lack of funds add funds
     * message sends and CID of the message returned
     * @param deal to ensure funds for
     * @return CID of add funds message if it was sent
     */
    outcome::result<boost::optional<CID>> ensureFunds(
        std::shared_ptr<ClientDeal> deal);

    outcome::result<void> verifyDealResponseSignature(
        const SignedResponse &response,
        const std::shared_ptr<ClientDeal> &deal);

    /**
     * Verifies if deal was published correctly
     * @param deal state with publish message cid set
     * @return true if published or false otherwise
     */
    outcome::result<bool> verifyDealPublished(std::shared_ptr<ClientDeal> deal);

    /**
     * Look up stream by proposal cid
     * @param proposal_cid - key to find stream
     * @return stream associated with proposal
     */
    outcome::result<std::shared_ptr<CborStream>> getStream(
        const CID &proposal_cid);

    /**
     * Finalize deal, close connection, clean up
     * @param deal - deal to clean up
     */
    void finalizeDeal(std::shared_ptr<ClientDeal> deal);

    /**
     * Creates all FSM transitions
     * @return vector of transitions for fsm
     */
    std::vector<ClientTransition> makeFSMTransitions();

    /**
     * @brief Handle open storage deal event
     * Attempts to ensure the client has enough funds for the deal being
     * proposed
     * @param deal  - current storage deal
     * @param event - ClientEventOpen
     * @param from  - STORAGE_DEAL_UNKNOWN
     * @param to    - STORAGE_DEAL_ENSURE_CLIENT_FUNDS
     */
    void onClientEventOpen(std::shared_ptr<ClientDeal> deal,
                           ClientEvent event,
                           StorageDealStatus from,
                           StorageDealStatus to);

    /**
     * @brief Handle initiate funding
     * @param deal  - current storage deal
     * @param event - ClientEventFundingInitiated
     * @param from  - STORAGE_DEAL_ENSURE_CLIENT_FUNDS
     * @param to    - STORAGE_DEAL_CLIENT_FUNDING
     */
    void onClientEventFundingInitiated(std::shared_ptr<ClientDeal> deal,
                                       ClientEvent event,
                                       StorageDealStatus from,
                                       StorageDealStatus to);

    /**
     * @brief Handle ensure funds
     * Propose deal
     * @param deal  - current storage deal
     * @param event - ClientEventFundsEnsured
     * @param from  - STORAGE_DEAL_ENSURE_CLIENT_FUNDS or
     * STORAGE_DEAL_CLIENT_FUNDING
     * @param to    - STORAGE_DEAL_FUNDS_ENSURED
     */
    void onClientEventFundsEnsured(std::shared_ptr<ClientDeal> deal,
                                   ClientEvent event,
                                   StorageDealStatus from,
                                   StorageDealStatus to);

    /**
     * @brief Handle deal proposal
     * @param deal  - current storage deal
     * @param event - ClientEventDealProposed
     * @param from  - STORAGE_DEAL_FUNDS_ENSURED
     * @param to    - STORAGE_DEAL_VALIDATING
     */
    void onClientEventDealProposed(std::shared_ptr<ClientDeal> deal,
                                   ClientEvent event,
                                   StorageDealStatus from,
                                   StorageDealStatus to);

    /**
     * @brief Handle deal reject
     * @param deal  - current storage deal
     * @param event - ClientEventDealRejected
     * @param from  - STORAGE_DEAL_VALIDATING
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onClientEventDealRejected(std::shared_ptr<ClientDeal> deal,
                                   ClientEvent event,
                                   StorageDealStatus from,
                                   StorageDealStatus to);

    /**
     * @brief Handle deal accepted
     * Validates that the provided deal has appeared on chain and references the
     * same ClientDeal
     * @param deal  - current storage deal
     * @param event - ClientEventDealAccepted
     * @param from  - STORAGE_DEAL_VALIDATING
     * @param to    - STORAGE_DEAL_PROPOSAL_ACCEPTED
     */
    void onClientEventDealAccepted(std::shared_ptr<ClientDeal> deal,
                                   ClientEvent event,
                                   StorageDealStatus from,
                                   StorageDealStatus to);

    /**
     * @brief Handle deal published
     * @param deal  - current storage deal
     * @param event - ClientEventDealPublished
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
     * @param to    - STORAGE_DEAL_SEALING
     */
    void onClientEventDealPublished(std::shared_ptr<ClientDeal> deal,
                                    ClientEvent event,
                                    StorageDealStatus from,
                                    StorageDealStatus to);

    /**
     * @brief Handle deal activation
     * @param deal  - current storage deal
     * @param event - ClientEventDealActivated
     * @param from  - STORAGE_DEAL_SEALING
     * @param to    - STORAGE_DEAL_ACTIVE
     */
    void onClientEventDealActivated(std::shared_ptr<ClientDeal> deal,
                                    ClientEvent event,
                                    StorageDealStatus from,
                                    StorageDealStatus to);

    /**
     * @brief Handle event fail
     * @param deal  - current storage deal
     * @param event - ClientEventFailed
     * @param from  - STORAGE_DEAL_FAILING
     * @param to    - STORAGE_DEAL_ERROR
     */
    void onClientEventFailed(std::shared_ptr<ClientDeal> deal,
                             ClientEvent event,
                             StorageDealStatus from,
                             StorageDealStatus to);

    /**
     * If error is present, closes connection and prints message
     * @tparam T - result type
     * @param res - result to check for error
     * @param on_error_msg - message to log on error
     * @param stream - stream to close on error
     * @param handler - error handler
     * @return true if
     */
    template <class T, class THandler>
    bool hasValue(outcome::result<T> res,
                  const std::string &on_error_msg,
                  const std::shared_ptr<CborStream> &stream,
                  const THandler &handler) {
      if (res.has_error()) {
        logger_->error(on_error_msg + " " + res.error().message());
        handler(res.error());
        closeStreamGracefully(stream, logger_);
        return false;
      }
      return true;
    };

    /** libp2p host */
    std::shared_ptr<CborHost> host_;
    std::shared_ptr<boost::asio::io_context> context_;

    std::shared_ptr<Api> api_;
    std::shared_ptr<PieceIO> piece_io_;
    std::shared_ptr<Discovery> discovery_;
    std::shared_ptr<DataTransfer> datatransfer_;

    // connection manager
    std::mutex connections_mutex_;
    std::map<CID, std::shared_ptr<CborStream>> connections_;

    /** State machine */
    std::shared_ptr<ClientFSM> fsm_;

    common::Logger logger_ = common::createLogger("StorageMarketClient");
  };

  /**
   * @brief Type of errors returned by Storage Market Client
   */
  enum class StorageMarketClientError {
    kWrongMiner = 1,
    kSignatureInvalid,
    kPieceDataNotSetManualTransfer,
    kPieceSizeGreaterSectorSize,
    kAddFundsCallError,
    kLocalDealNotFound,
    kStreamLookupError,
  };

}  // namespace fc::markets::storage::client

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::client,
                          StorageMarketClientError);

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP
