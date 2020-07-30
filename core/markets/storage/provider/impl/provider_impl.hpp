/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP

#include <libp2p/host/host.hpp>
#include <mutex>
#include "api/miner_api.hpp"
#include "common/libp2p/cbor_host.hpp"
#include "common/logger.hpp"
#include "data_transfer/manager.hpp"
#include "fsm/fsm.hpp"
#include "markets/common.hpp"
#include "markets/pieceio/pieceio.hpp"
#include "markets/storage/chain_events/chain_events.hpp"
#include "markets/storage/provider/provider.hpp"
#include "markets/storage/provider/provider_events.hpp"
#include "markets/storage/provider/stored_ask.hpp"
#include "storage/filestore/filestore.hpp"
#include "storage/keystore/keystore.hpp"
#include "storage/piece/piece_storage.hpp"

namespace fc::markets::storage::provider {
  using api::MinerApi;
  using api::PieceLocation;
  using chain_events::ChainEvents;
  using common::libp2p::CborHost;
  using common::libp2p::CborStream;
  using fc::storage::filestore::FileStore;
  using fc::storage::filestore::Path;
  using fc::storage::piece::PieceStorage;
  using libp2p::Host;
  using pieceio::PieceIO;
  using primitives::BigInt;
  using primitives::EpochDuration;
  using primitives::GasAmount;
  using primitives::sector::RegisteredProof;
  using ProviderTransition =
      fsm::Transition<ProviderEvent, StorageDealStatus, MinerDeal>;
  using ProviderFSM = fsm::FSM<ProviderEvent, StorageDealStatus, MinerDeal>;
  using DataTransfer = data_transfer::Manager;

  const EpochDuration kDefaultDealAcceptanceBuffer{100};

  const Path kFilestoreTempDir = "/tmp/fuhon/storage-market/";

  class StorageProviderImpl
      : public StorageProvider,
        public std::enable_shared_from_this<StorageProviderImpl> {
   public:
    StorageProviderImpl(const RegisteredProof &registered_proof,
                        std::shared_ptr<Host> host,
                        std::shared_ptr<boost::asio::io_context> context,
                        std::shared_ptr<Datastore> datastore,
                        std::shared_ptr<Api> api,
                        std::shared_ptr<MinerApi> miner_api,
                        std::shared_ptr<ChainEvents> events,
                        const Address &miner_actor_address,
                        std::shared_ptr<PieceIO> piece_io,
                        std::shared_ptr<FileStore> filestore);

    auto init() -> outcome::result<void> override;

    auto start() -> outcome::result<void> override;

    auto stop() -> outcome::result<void> override;

    auto addAsk(const TokenAmount &price, ChainEpoch duration)
        -> outcome::result<void> override;

    auto listAsks(const Address &address)
        -> outcome::result<std::vector<SignedStorageAsk>> override;

    auto getDeal(const CID &proposal_cid) const
        -> outcome::result<MinerDeal> override;

    auto addStorageCollateral(const TokenAmount &amount)
        -> outcome::result<void> override;

    auto getStorageCollateral() -> outcome::result<TokenAmount> override;

    auto importDataForDeal(const CID &proposal_cid, const Buffer &data)
        -> outcome::result<void> override;

   private:
    /**
     * Handle incoming ask stream
     * @param stream
     */
    auto handleAskStream(const std::shared_ptr<CborStream> &stream) -> void;

    /**
     * Handle incoming deal proposal stream
     * @param stream
     */
    auto handleDealStream(const std::shared_ptr<CborStream> &stream) -> void;

    /**
     * Verify client signature for deal proposal
     * @param deal to verify
     * @return true if verified or false otherwise
     */
    outcome::result<bool> verifyDealProposal(
        std::shared_ptr<MinerDeal> deal) const;

    /**
     * Ensure provider has enough funds
     * @param deal - storage deal
     * @return cid of funding message if it was sent
     */
    outcome::result<boost::optional<CID>> ensureProviderFunds(
        std::shared_ptr<MinerDeal> deal);

    /**
     * Publish storage deal
     * @param deal to publish
     * @return CID of message sent
     */
    outcome::result<CID> publishDeal(std::shared_ptr<MinerDeal> deal);

    /**
     * Send signed response to storage deal proposal and close connection
     * @param deal - state of deal
     */
    outcome::result<void> sendSignedResponse(std::shared_ptr<MinerDeal> deal);

    /**
     * Locate piece for deal
     * @param deal - activated deal
     * @return piece info with location
     */
    outcome::result<PieceLocation> locatePiece(std::shared_ptr<MinerDeal> deal);

    /**
     * Records sector information about an activated deal so that the data can
     * be retrieved later
     * @param deal - activated deal
     * @param piece_location - piece location
     * @return error in case of failure
     */
    outcome::result<void> recordPieceInfo(std::shared_ptr<MinerDeal> deal,
                                          const PieceLocation &piece_location);

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
    outcome::result<void> finalizeDeal(std::shared_ptr<MinerDeal> deal);

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
     * @brief Handle event handoff
     * @param deal  - current storage deal
     * @param event - ProviderEventDealHandedOff
     * @param from  - STORAGE_DEAL_STAGED
     * @param to    - STORAGE_DEAL_SEALING
     */
    void onProviderEventDealHandedOff(std::shared_ptr<MinerDeal> deal,
                                      ProviderEvent event,
                                      StorageDealStatus from,
                                      StorageDealStatus to);

    /**
     * @brief Handle event deal activation
     * @param deal  - current storage deal
     * @param event - ProviderEventDealActivated
     * @param from  - STORAGE_DEAL_SEALING
     * @param to    - STORAGE_DEAL_ACTIVE
     */
    void onProviderEventDealActivated(std::shared_ptr<MinerDeal> deal,
                                      ProviderEvent event,
                                      StorageDealStatus from,
                                      StorageDealStatus to);

    /**
     * @brief Handle event deal completed
     * @param deal  - current storage deal
     * @param event - ProviderEventDealCompleted
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
                  const std::shared_ptr<CborStream> &stream) {
      if (res.has_error()) {
        logger_->error(on_error_msg + res.error().message());
        closeStreamGracefully(stream, logger_);
        return false;
      }
      return true;
    };

    RegisteredProof registered_proof_;

    // coonnection manager
    std::mutex connections_mutex_;
    std::map<CID, std::shared_ptr<CborStream>> connections_;

    /** State machine */
    std::shared_ptr<ProviderFSM> fsm_;

    std::shared_ptr<CborHost> host_;
    std::shared_ptr<boost::asio::io_context> context_;
    std::shared_ptr<StoredAsk> stored_ask_;
    std::shared_ptr<Api> api_;
    std::shared_ptr<MinerApi> miner_api_;
    std::shared_ptr<ChainEvents> chain_events_;
    Address miner_actor_address_;
    std::shared_ptr<PieceIO> piece_io_;
    std::shared_ptr<PieceStorage> piece_storage_;
    std::shared_ptr<FileStore> filestore_;
    std::shared_ptr<DataTransfer> datatransfer_;

    common::Logger logger_ = common::createLogger("StorageMarketProvider");
  };

  /**
   * @brief Type of errors returned by Storage Market Provider
   */
  enum class StorageMarketProviderError {
    kLocalDealNotFound = 1,
    kPieceCIDDoesNotMatch
  };

}  // namespace fc::markets::storage::provider

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::provider,
                          StorageMarketProviderError);

#endif  // CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP
