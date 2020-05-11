/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP

#include <libp2p/host/host.hpp>
#include "api/api.hpp"
#include "common/logger.hpp"
#include "data_transfer/manager.hpp"
#include "fsm/fsm.hpp"
#include "host/context/host_context.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/client/client.hpp"
#include "markets/storage/client/client_events.hpp"
#include "markets/storage/network/libp2p_storage_market_network.hpp"
#include "storage/filestore/filestore.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/keystore/keystore.hpp"

namespace fc::markets::storage::client {

  using api::Api;
  using fc::storage::filestore::FileStore;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::keystore::KeyStore;
  using fsm::FSM;
  using libp2p::Host;
  using network::Libp2pStorageMarketNetwork;
  using pieceio::PieceIO;
  using ClientTransition =
      fsm::Transition<ClientEvent, StorageDealStatus, ClientDeal>;
  using ClientFSM = fsm::FSM<ClientEvent, StorageDealStatus, ClientDeal>;
  using Ticks = libp2p::protocol::Scheduler::Ticks;

  class ClientImpl : public Client,
                     public std::enable_shared_from_this<ClientImpl> {
   public:
    ClientImpl(std::shared_ptr<Host> host,
               std::shared_ptr<boost::asio::io_context> context,
               std::shared_ptr<Api> api,
               std::shared_ptr<KeyStore> keystore,
               std::shared_ptr<PieceIO> piece_io);

    void init() override;

    void run() override;

    void stop() override;

    outcome::result<std::vector<StorageProviderInfo>> listProviders()
        const override;

    outcome::result<std::vector<StorageDeal>> listDeals(
        const Address &address) const override;

    outcome::result<std::vector<ClientDeal>> listLocalDeals() const override;

    outcome::result<ClientDeal> getLocalDeal(const CID &cid) const override;

    void getAsk(const StorageProviderInfo &info,
                const SignedAskHandler &signed_ask_handler) const override;

    outcome::result<ProposeStorageDealResult> proposeStorageDeal(
        const Address &address,
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
     * Creates all FSM transitions
     * @return vector of transitions for fsm
     */
    std::vector<ClientTransition> makeFSMTransitions();

    /**
     * @brief Handle open storage deal event
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
     * @brief Handle open storage deal event
     * @param deal  - current storage deal
     * @param event - ClientEventOpenStreamError
     * @param from  - STORAGE_DEAL_UNKNOWN
     * @param to    - STORAGE_DEAL_ENSURE_CLIENT_FUNDS
     */
    void onClientEventOpenStreamError(std::shared_ptr<ClientDeal> deal,
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
     * @brief Handle ensure funds fail
     * @param deal  - current storage deal
     * @param event - ClientEventEnsureFundsFailed
     * @param from  - STORAGE_DEAL_CLIENT_FUNDING or
     * STORAGE_DEAL_ENSURE_CLIENT_FUNDS
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onClientEventEnsureFundsFailed(std::shared_ptr<ClientDeal> deal,
                                        ClientEvent event,
                                        StorageDealStatus from,
                                        StorageDealStatus to);

    /**
     * @brief Handle ensure funds
     * @param deal  - current storage deal
     * @param event - ClientEventEnsureFundsFailed
     * @param from  - STORAGE_DEAL_ENSURE_CLIENT_FUNDS or
     * STORAGE_DEAL_CLIENT_FUNDING
     * @param to    - STORAGE_DEAL_FUNDS_ENSURED
     */
    void onClientEventFundsEnsured(std::shared_ptr<ClientDeal> deal,
                                   ClientEvent event,
                                   StorageDealStatus from,
                                   StorageDealStatus to);

    /**
     * @brief Handle write proposal fail
     * @param deal  - current storage deal
     * @param event - ClientEventEnsureFundsFailed
     * @param from  - STORAGE_DEAL_FUNDS_ENSURED
     * @param to    - STORAGE_DEAL_ERROR
     */
    void onClientEventWriteProposalFailed(std::shared_ptr<ClientDeal> deal,
                                          ClientEvent event,
                                          StorageDealStatus from,
                                          StorageDealStatus to);

    /**
     * @brief Handle deal proposal
     * @param deal  - current storage deal
     * @param event - ClientEventEnsureFundsFailed
     * @param from  - STORAGE_DEAL_FUNDS_ENSURED
     * @param to    - STORAGE_DEAL_VALIDATING
     */
    void onClientEventDealProposed(std::shared_ptr<ClientDeal> deal,
                                   ClientEvent event,
                                   StorageDealStatus from,
                                   StorageDealStatus to);

    /**
     * @brief Handle deal stream lookup error
     * @param deal  - current storage deal
     * @param event - ClientEventDealStreamLookupErrored
     * @param from  - any state
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onClientEventDealStreamLookupErrored(std::shared_ptr<ClientDeal> deal,
                                              ClientEvent event,
                                              StorageDealStatus from,
                                              StorageDealStatus to);

    /**
     * @brief Handle read response fail
     * @param deal  - current storage deal
     * @param event - ClientEventReadResponseFailed
     * @param from  - STORAGE_DEAL_VALIDATING
     * @param to    - STORAGE_DEAL_ERROR
     */
    void onClientEventReadResponseFailed(std::shared_ptr<ClientDeal> deal,
                                         ClientEvent event,
                                         StorageDealStatus from,
                                         StorageDealStatus to);

    /**
     * @brief Handle response verification fail
     * @param deal  - current storage deal
     * @param event - ClientEventResponseVerificationFailed
     * @param from  - STORAGE_DEAL_VALIDATING
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onClientEventResponseVerificationFailed(
        std::shared_ptr<ClientDeal> deal,
        ClientEvent event,
        StorageDealStatus from,
        StorageDealStatus to);

    /**
     * @brief Handle response deal didn't match
     * @param deal  - current storage deal
     * @param event - ClientEventResponseDealDidNotMatch
     * @param from  - STORAGE_DEAL_VALIDATING
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onClientEventResponseDealDidNotMatch(std::shared_ptr<ClientDeal> deal,
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
     * @brief Handle stream close error
     * @param deal  - current storage deal
     * @param event - ClientEventStreamCloseError
     * @param from  - any
     * @param to    - STORAGE_DEAL_ERROR
     */
    void onClientEventStreamCloseError(std::shared_ptr<ClientDeal> deal,
                                       ClientEvent event,
                                       StorageDealStatus from,
                                       StorageDealStatus to);

    /**
     * @brief Handle deal publish fail
     * @param deal  - current storage deal
     * @param event - ClientEventDealPublishFailed
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
     * @param to    - STORAGE_DEAL_ERROR
     */
    void onClientEventDealPublishFailed(std::shared_ptr<ClientDeal> deal,
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
     * @brief Handle activation fail
     * @param deal  - current storage deal
     * @param event - ClientEventDealActivationFailed
     * @param from  - STORAGE_DEAL_SEALING
     * @param to    - STORAGE_DEAL_ERROR
     */
    void onClientEventDealActivationFailed(std::shared_ptr<ClientDeal> deal,
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
                  const THandler &handler) const {
      if (res.has_error()) {
        logger_->error(on_error_msg + res.error().message());
        handler(res.error());
        network_->closeStreamGracefully(stream);
        return false;
      }
      return true;
    };

    /** libp2p host */
    std::shared_ptr<Host> host_;
    std::shared_ptr<boost::asio::io_context> context_;

    std::shared_ptr<Api> api_;
    std::shared_ptr<KeyStore> keystore_;
    std::shared_ptr<PieceIO> piece_io_;
    std::shared_ptr<StorageMarketNetwork> network_;

    // TODO
    // discovery

    // TODO
    // connection manager
    std::map<CID, std::shared_ptr<CborStream>> connections_;

    /** State machine */
    std::shared_ptr<ClientFSM> fsm_;

    /**
     * Set of local deals proposal_cid -> client deal, handled by fsm
     */
    std::map<CID, std::shared_ptr<ClientDeal>> local_deals_;

    common::Logger logger_ = common::createLogger("StorageMarketClient");
  };

  /**
   * @brief Type of errors returned by Storage Market Client
   */
  enum class StorageMarketClientError {
    WRONG_MINER = 1,
    SIGNATURE_INVALID,
    PIECE_DATA_NOT_SET_MANUAL_TRANSFER,
    PIECE_SIZE_GREATER_SECTOR_SIZE,
    ADD_FUNDS_CALL_ERROR,
    LOCAL_DEAL_NOT_FOUND
  };

}  // namespace fc::markets::storage::client

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::client,
                          StorageMarketClientError);

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP
