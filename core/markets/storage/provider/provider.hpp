/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP

#include <libp2p/host/host.hpp>
#include "common/logger.hpp"
#include "fsm/fsm.hpp"
#include "markets/pieceio/pieceio.hpp"
#include "markets/storage/network/libp2p_storage_market_network.hpp"
#include "markets/storage/provider.hpp"
#include "markets/storage/provider/provider_events.hpp"
#include "markets/storage/provider/stored_ask.hpp"
#include "markets/storage/storage_receiver.hpp"

namespace fc::markets::storage::provider {
  using libp2p::Host;
  using network::Libp2pStorageMarketNetwork;
  using pieceio::PieceIO;
  using primitives::sector::RegisteredProof;
  using ProviderTransition =
      fsm::Transition<ProviderEvent, StorageDealStatus, MinerDeal>;
  using ProviderFSM = fsm::FSM<ProviderEvent, StorageDealStatus, MinerDeal>;

  class StorageProviderImpl
      : public StorageProvider,
        public StorageReceiver,
        public std::enable_shared_from_this<StorageProviderImpl> {
   public:
    StorageProviderImpl(const RegisteredProof &registered_proof,
                        std::shared_ptr<Host> host,
                        std::shared_ptr<boost::asio::io_context> context,
                        std::shared_ptr<KeyStore> keystore,
                        std::shared_ptr<Datastore> datastore,
                        std::shared_ptr<Api> api,
                        const Address &actor_address,
                        std::shared_ptr<PieceIO> piece_io);

    auto init() -> void override;

    auto start() -> outcome::result<void> override;

    auto addAsk(const TokenAmount &price, ChainEpoch duration)
        -> outcome::result<void> override;

    auto listAsks(const Address &address)
        -> outcome::result<std::vector<SignedStorageAsk>> override;

    auto listDeals() -> outcome::result<std::vector<StorageDeal>> override;

    auto listIncompleteDeals()
        -> outcome::result<std::vector<MinerDeal>> override;

    auto getDeal(const CID &proposal_cid) const
        -> outcome::result<std::shared_ptr<MinerDeal>> override;

    auto addStorageCollateral(const TokenAmount &amount)
        -> outcome::result<void> override;

    auto getStorageCollateral() -> outcome::result<TokenAmount> override;

    auto importDataForDeal(const CID &proposal_cid, const Buffer &data)
        -> outcome::result<void> override;

   protected:
    auto handleAskStream(const std::shared_ptr<CborStream> &stream)
        -> void override;

    auto handleDealStream(const std::shared_ptr<CborStream> &stream)
        -> void override;

   private:
    outcome::result<boost::optional<CID>> ensureFunds(
        std::shared_ptr<MinerDeal> deal);

    /**
     * Creates all FSM transitions
     * @return vector of transitions for fsm
     */
    std::vector<ProviderTransition> makeFSMTransitions();

    /**
     * @brief Handle event open deal
     * Validates deal proposal
     * @param deal  - current storage deal
     * @param event - ProviderEventOpen
     * @param from  - STORAGE_DEAL_UNKNOWN
     * @param to    - STORAGE_DEAL_VALIDATING
     */
    void onProviderEventOpen(std::shared_ptr<MinerDeal> deal,
                             ProviderEvent event,
                             StorageDealStatus from,
                             StorageDealStatus to);

    /**
     * @brief Handle event node error
     * @param deal  - current storage deal
     * @param event - ProviderEventNodeErrored
     * @param from  - any
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventNodeErrored(std::shared_ptr<MinerDeal> deal,
                                    ProviderEvent event,
                                    StorageDealStatus from,
                                    StorageDealStatus to);

    /**
     * @brief Handle event deal rejected
     * @param deal  - current storage deal
     * @param event - ProviderEventOpen
     * @param from  - STORAGE_DEAL_VALIDATING or STORAGE_DEAL_VERIFY_DATA
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventDealRejected(std::shared_ptr<MinerDeal> deal,
                                     ProviderEvent event,
                                     StorageDealStatus from,
                                     StorageDealStatus to);

    /**
     * @brief Handle event deal accepted
     * @param deal  - current storage deal
     * @param event - ProviderEventDealAccepted
     * @param from  - STORAGE_DEAL_VALIDATING
     * @param to    - STORAGE_DEAL_PROPOSAL_ACCEPTED
     */
    void onProviderEventDealAccepted(std::shared_ptr<MinerDeal> deal,
                                     ProviderEvent event,
                                     StorageDealStatus from,
                                     StorageDealStatus to);

    /**
     * @brief Handle event waiting for manual data
     * @param deal  - current storage deal
     * @param event - ProviderEventWaitingForManualData
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
     * @param to    - STORAGE_DEAL_WAITING_FOR_DATA
     */
    void onProviderEventWaitingForManualData(std::shared_ptr<MinerDeal> deal,
                                             ProviderEvent event,
                                             StorageDealStatus from,
                                             StorageDealStatus to);

    /**
     * @brief Handle event insufficient funds
     * @param deal  - current storage deal
     * @param event - ProviderEventWaitingForManualData
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
     * @param to    - STORAGE_DEAL_WAITING_FOR_DATA
     */
    void onProviderEventInsufficientFunds(std::shared_ptr<MinerDeal> deal,
                                          ProviderEvent event,
                                          StorageDealStatus from,
                                          StorageDealStatus to);

    /**
     * @brief Handle event funding initiated
     * @param deal  - current storage deal
     * @param event - ProviderEventWaitingForManualData
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
     * @param to    - STORAGE_DEAL_WAITING_FOR_DATA
     */
    void onProviderEventFundingInitiated(std::shared_ptr<MinerDeal> deal,
                                         ProviderEvent event,
                                         StorageDealStatus from,
                                         StorageDealStatus to);

    /**
     * @brief Handle event funded
     * @param deal  - current storage deal
     * @param event - ProviderEventWaitingForManualData
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
     * @param to    - STORAGE_DEAL_WAITING_FOR_DATA
     */
    void onProviderEventFunded(std::shared_ptr<MinerDeal> deal,
                               ProviderEvent event,
                               StorageDealStatus from,
                               StorageDealStatus to);

    /**
     * @brief Handle event data transfer failed
     * @param deal  - current storage deal
     * @param event - ProviderEventDataTransferFailed
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED or
     * STORAGE_DEAL_TRANSFERRING
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventDataTransferFailed(std::shared_ptr<MinerDeal> deal,
                                           ProviderEvent event,
                                           StorageDealStatus from,
                                           StorageDealStatus to);

    /**
     * @brief Handle event data transfer initiated
     * @param deal  - current storage deal
     * @param event - ProviderEventDataTransferInitiated
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
     * @param to    - STORAGE_DEAL_TRANSFERRING
     */
    void onProviderEventDataTransferInitiated(std::shared_ptr<MinerDeal> deal,
                                              ProviderEvent event,
                                              StorageDealStatus from,
                                              StorageDealStatus to);

    /**
     * @brief Handle event data transfer completed
     * @param deal  - current storage deal
     * @param event - ProviderEventDataTransferCompleted
     * @param from  - STORAGE_DEAL_TRANSFERRING
     * @param to    - STORAGE_DEAL_VERIFY_DATA
     */
    void onProviderEventDataTransferCompleted(std::shared_ptr<MinerDeal> deal,
                                              ProviderEvent event,
                                              StorageDealStatus from,
                                              StorageDealStatus to);

    /**
     * @brief Handle event data manual data received
     * @param deal  - current storage deal
     * @param event - ProviderEventDataTransferCompleted
     * @param from  - STORAGE_DEAL_TRANSFERRING
     * @param to    - STORAGE_DEAL_VERIFY_DATA
     */
    void onProviderEventManualDataReceived(std::shared_ptr<MinerDeal> deal,
                                           ProviderEvent event,
                                           StorageDealStatus from,
                                           StorageDealStatus to);

    /**
     * @brief Handle event generate piece CID failed
     * @param deal  - current storage deal
     * @param event - ProviderEventGeneratePieceCIDFailed
     * @param from  - STORAGE_DEAL_VERIFY_DATA
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventGeneratePieceCIDFailed(std::shared_ptr<MinerDeal> deal,
                                               ProviderEvent event,
                                               StorageDealStatus from,
                                               StorageDealStatus to);

    /**
     * @brief Handle event data verified
     * @param deal  - current storage deal
     * @param event - ProviderEventVerifiedData
     * @param from  - STORAGE_DEAL_VERIFY_DATA
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventVerifiedData(std::shared_ptr<MinerDeal> deal,
                                     ProviderEvent event,
                                     StorageDealStatus from,
                                     StorageDealStatus to);

    /**
     * @brief Handle event generate piece CID failed
     * @param deal  - current storage deal
     * @param event - ProviderEventSendResponseFailed
     * @param from  - STORAGE_DEAL_VERIFY_DATA
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventSendResponseFailed(std::shared_ptr<MinerDeal> deal,
                                           ProviderEvent event,
                                           StorageDealStatus from,
                                           StorageDealStatus to);

    /**
     * @brief Handle event deal publish initiated
     * @param deal  - current storage deal
     * @param event - ProviderEventDealPublishInitiated
     * @param from  - STORAGE_DEAL_VERIFY_DATA
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventDealPublishInitiated(std::shared_ptr<MinerDeal> deal,
                                             ProviderEvent event,
                                             StorageDealStatus from,
                                             StorageDealStatus to);

    /**
     * @brief Handle event deal published
     * @param deal  - current storage deal
     * @param event - ProviderEventDealPublished
     * @param from  - STORAGE_DEAL_VERIFY_DATA or STORAGE_DEAL_WAITING_FOR_DATA
     * @param to    - STORAGE_DEAL_ENSURE_PROVIDER_FUNDS
     */
    void onProviderEventDealPublished(std::shared_ptr<MinerDeal> deal,
                                      ProviderEvent event,
                                      StorageDealStatus from,
                                      StorageDealStatus to);

    /**
     * @brief Handle event deal publish error
     * @param deal  - current storage deal
     * @param event - ProviderEventDealPublishError
     * @param from  - STORAGE_DEAL_VERIFY_DATA or STORAGE_DEAL_WAITING_FOR_DATA
     * @param to    - STORAGE_DEAL_ENSURE_PROVIDER_FUNDS
     */
    void onProviderEventDealPublishError(std::shared_ptr<MinerDeal> deal,
                                         ProviderEvent event,
                                         StorageDealStatus from,
                                         StorageDealStatus to);

    /**
     * @brief Handle event filestore error
     * @param deal  - current storage deal
     * @param event - ProviderEventFileStoreErrored
     * @param from  - STORAGE_DEAL_VERIFY_DATA or STORAGE_DEAL_WAITING_FOR_DATA
     * @param to    - STORAGE_DEAL_ENSURE_PROVIDER_FUNDS
     */
    void onProviderEventFileStoreErrored(std::shared_ptr<MinerDeal> deal,
                                         ProviderEvent event,
                                         StorageDealStatus from,
                                         StorageDealStatus to);

    /**
     * @brief Handle event hand off failed
     * @param deal  - current storage deal
     * @param event - ProviderEventDealHandoffFailed
     * @param from  - STORAGE_DEAL_VERIFY_DATA or STORAGE_DEAL_WAITING_FOR_DATA
     * @param to    - STORAGE_DEAL_ENSURE_PROVIDER_FUNDS
     */
    void onProviderEventDealHandoffFailed(std::shared_ptr<MinerDeal> deal,
                                          ProviderEvent event,
                                          StorageDealStatus from,
                                          StorageDealStatus to);

    /**
     * @brief Handle event handoff
     * @param deal  - current storage deal
     * @param event - ProviderEventDealHandoffFailed
     * @param from  - STORAGE_DEAL_STAGED
     * @param to    - STORAGE_DEAL_SEALING
     */
    void onProviderEventDealHandedOff(std::shared_ptr<MinerDeal> deal,
                                      ProviderEvent event,
                                      StorageDealStatus from,
                                      StorageDealStatus to);

    /**
     * @brief Handle event deal activation failed
     * @param deal  - current storage deal
     * @param event - ProviderEventDealActivationFailed
     * @param from  - STORAGE_DEAL_SEALING
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventDealActivationFailed(std::shared_ptr<MinerDeal> deal,
                                             ProviderEvent event,
                                             StorageDealStatus from,
                                             StorageDealStatus to);

    /**
     * @brief Handle event unable to locate piece
     * @param deal  - current storage deal
     * @param event - ProviderEventDealActivationFailed
     * @param from  - STORAGE_DEAL_SEALING
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventUnableToLocatePiece(std::shared_ptr<MinerDeal> deal,
                                            ProviderEvent event,
                                            StorageDealStatus from,
                                            StorageDealStatus to);

    /**
     * @brief Handle event deal activation
     * @param deal  - current storage deal
     * @param event - ProviderEventDealActivationFailed
     * @param from  - STORAGE_DEAL_SEALING
     * @param to    - STORAGE_DEAL_ACTIVE
     */
    void onProviderEventDealActivated(std::shared_ptr<MinerDeal> deal,
                                      ProviderEvent event,
                                      StorageDealStatus from,
                                      StorageDealStatus to);

    /**
     * @brief Handle event piece store error
     * @param deal  - current storage deal
     * @param event - ProviderEventPieceStoreErrored
     * @param from  - STORAGE_DEAL_ACTIVE
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventPieceStoreErrored(std::shared_ptr<MinerDeal> deal,
                                          ProviderEvent event,
                                          StorageDealStatus from,
                                          StorageDealStatus to);

    /**
     * @brief Handle event read metadata error
     * @param deal  - current storage deal
     * @param event - ProviderEventReadMetadataErrored
     * @param from  - STORAGE_DEAL_ACTIVE
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventReadMetadataErrored(std::shared_ptr<MinerDeal> deal,
                                            ProviderEvent event,
                                            StorageDealStatus from,
                                            StorageDealStatus to);

    /**
     * @brief Handle event deal completed
     * @param deal  - current storage deal
     * @param event - ProviderEventReadMetadataErrored
     * @param from  - STORAGE_DEAL_ACTIVE
     * @param to    - STORAGE_DEAL_FAILING
     */
    void onProviderEventDealCompleted(std::shared_ptr<MinerDeal> deal,
                                      ProviderEvent event,
                                      StorageDealStatus from,
                                      StorageDealStatus to);

    /**
     * @brief Handle event failed
     * @param deal  - current storage deal
     * @param event - ProviderEventFailed
     * @param from  - STORAGE_DEAL_FAILING
     * @param to    - STORAGE_DEAL_ERROR
     */
    void onProviderEventFailed(std::shared_ptr<MinerDeal> deal,
                               ProviderEvent event,
                               StorageDealStatus from,
                               StorageDealStatus to);

    /**
     * If error is present, closes connection and prints message
     * @tparam T - result type
     * @param res - result to check for error
     * @param on_error_msg - message to log on error
     * @param stream - stream to close on error
     * @return true if
     */
    template <class T>
    bool hasValue(outcome::result<T> res,
                  const std::string &on_error_msg,
                  const std::shared_ptr<CborStream> &stream) const {
      if (res.has_error()) {
        logger_->error(on_error_msg + res.error().message());
        network_->closeStreamGracefully(stream);
        return false;
      }
      return true;
    };

    RegisteredProof registered_proof_;

    std::map<CID, std::shared_ptr<CborStream>> connections_;

    /**
     * Set of local deals proposal_cid -> client deal, handled by fsm
     */
    std::map<CID, std::shared_ptr<MinerDeal>> local_deals_;

    /** State machine */
    std::shared_ptr<ProviderFSM> fsm_;

    /**
     * Closes stream and handles close result
     * @param stream to close
     */
    void closeStreamGracefully(const std::shared_ptr<CborStream> &stream) const;

    std::shared_ptr<Host> host_;
    std::shared_ptr<boost::asio::io_context> context_;
    std::shared_ptr<StoredAsk> stored_ask_;
    std::shared_ptr<Api> api_;

    std::shared_ptr<StorageMarketNetwork> network_;
    std::shared_ptr<PieceIO> piece_io_;

    common::Logger logger_ = common::createLogger("StorageMarketProvider");
  };

  /**
   * @brief Type of errors returned by Storage Market Provider
   */
  enum class StorageMarketProviderError {
    LOCAL_DEAL_NOT_FOUND = 1,
    PIECE_CID_DOESNT_MATCH
  };

}  // namespace fc::markets::storage::provider

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::provider,
                          StorageMarketProviderError);

#endif  // CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP
