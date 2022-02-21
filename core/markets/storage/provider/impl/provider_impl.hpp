/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/host/host.hpp>
#include <mutex>

#include "api/storage_miner/storage_api.hpp"
#include "common/logger.hpp"
#include "data_transfer/dt.hpp"
#include "fsm/fsm.hpp"
#include "markets/common.hpp"
#include "markets/pieceio/pieceio.hpp"
#include "markets/storage/chain_events/chain_events.hpp"
#include "markets/storage/provider/provider.hpp"
#include "markets/storage/provider/provider_events.hpp"
#include "markets/storage/provider/stored_ask.hpp"
#include "markets/storage/status_protocol.hpp"
#include "sectorblocks/blocks.hpp"
#include "storage/filestore/filestore.hpp"
#include "storage/keystore/keystore.hpp"
#include "storage/piece/piece_storage.hpp"
#include "vm/actor/builtin/types/market/deal_info_manager/deal_info_manager.hpp"

#define FSM_HANDLE_DEFINITION(name)                           \
  void name(const std::shared_ptr<DealContext> &deal_context, \
            ProviderEvent event,                              \
            StorageDealStatus from,                           \
            StorageDealStatus to)

namespace fc::markets::storage::provider {
  using api::MsgWait;
  using api::PieceLocation;
  using chain_events::ChainEvents;
  using common::libp2p::CborStream;
  using data_transfer::DataTransfer;
  using fc::storage::filestore::FileStore;
  using fc::storage::piece::PieceStorage;
  using libp2p::Host;
  using pieceio::PieceIO;
  using primitives::BigInt;
  using primitives::EpochDuration;
  using primitives::GasAmount;
  using primitives::sector::RegisteredSealProof;
  using sectorblocks::SectorBlocks;
  using vm::actor::builtin::types::market::deal_info_manager::DealInfoManager;

  const EpochDuration kDefaultDealAcceptanceBuffer{100};

  class StorageProviderImpl
      : public StorageProvider,
        public std::enable_shared_from_this<StorageProviderImpl> {
   public:
    StorageProviderImpl(std::shared_ptr<Host> host,
                        IpldPtr ipld,
                        std::shared_ptr<DataTransfer> datatransfer,
                        std::shared_ptr<StoredAsk> stored_ask,
                        std::shared_ptr<boost::asio::io_context> context,
                        std::shared_ptr<PieceStorage> piece_storage,
                        std::shared_ptr<FullNodeApi> api,
                        std::shared_ptr<SectorBlocks> sector_blocks,
                        std::shared_ptr<ChainEvents> events,
                        Address miner_actor_address,
                        std::shared_ptr<PieceIO> piece_io,
                        std::shared_ptr<FileStore> filestore,
                        std::shared_ptr<DealInfoManager> deal_info_manager);

    auto init() -> outcome::result<void> override;

    auto start() -> outcome::result<void> override;

    auto stop() -> outcome::result<void> override;

    auto getDeal(const CID &proposal_cid) const
        -> outcome::result<MinerDeal> override;

    auto getLocalDeals() const
        -> outcome::result<std::vector<MinerDeal>> override;

    auto importDataForDeal(const CID &proposal_cid,
                           const boost::filesystem::path &path)
        -> outcome::result<void> override;

    outcome::result<Signature> sign(const Bytes &input) const;

   private:
    struct DealContext {
      std::shared_ptr<MinerDeal> deal;
      std::string protocol;
      boost::optional<PieceLocation> maybe_piece_location{};
    };

    using ProviderTransition =
        fsm::Transition<ProviderEvent, void, StorageDealStatus, DealContext>;
    using ProviderFSM =
        fsm::FSM<ProviderEvent, void, StorageDealStatus, DealContext>;

    std::shared_ptr<DealContext> getDealContextPtr(
        const CID &proposal_cid) const;

    /**
     * Sets deal ask protocol handlers for different protocol versions.
     * @tparam AskRequestType - ask request type for the protocol version
     * @tparam AskResponseType - ask response type for the protocol
     * @param protocol - protocol string
     */
    template <typename AskRequestType, typename AskResponseType>
    inline void setAskHandler(const std::string &protocol) {
      host_->setProtocolHandler(
          protocol, [stored_ask{weaken(stored_ask_)}](auto _stream) {
            auto stream{std::make_shared<common::libp2p::CborStream>(_stream)};
            stream->template read<AskRequestType>([stored_ask,
                                                   stream](auto request) {
              if (request) {
                if (auto asker{stored_ask.lock()}) {
                  if (auto ask{asker->getAsk(request.value().miner)}) {
                    return stream->write(AskResponseType{{ask.value()}},
                                         [stream](auto) { stream->close(); });
                  }
                }
              }
              stream->stream()->reset();
            });
          });
    }

    /**
     * Sets deal status protocol handler for different protocol versions.
     * @tparam DealStatusRequestType - request for the protocol
     * @tparam DealStatusResponseType - reponse type for protocol
     * @param protocol - protocol string
     */
    template <typename DealStatusRequestType, typename DealStatusResponseType>
    inline void setDealStatusHandler(const std::string &protocol) {
      host_->setProtocolHandler(
          protocol, [provider_ptr{weak_from_this()}](auto _stream) {
            auto stream{std::make_shared<common::libp2p::CborStream>(_stream)};
            stream->template read<DealStatusRequestType>([provider_ptr, stream](
                                                             auto _request) {
              if (_request) {
                auto &request{_request.value()};
                if (auto provider{provider_ptr.lock()}) {
                  const auto maybe_response{
                      provider
                          ->prepareDealStateResponse<DealStatusResponseType>(
                              request)};
                  if (maybe_response) {
                    stream->write(maybe_response.value(),
                                  [stream](auto) { stream->close(); });
                  } else {
                    provider->logger_->error(
                        "Cannot create deal status response: "
                        + maybe_response.error().message());
                  }
                }
              }
              stream->stream()->reset();
            });
          });
    }

    /**
     * Creates deal status response for particular protocol.
     * @tparam DealStatusResponseType - response for protocol
     * @param request
     * @return response for protocol
     */
    template <typename DealStatusResponseType>
    outcome::result<DealStatusResponseType> prepareDealStateResponse(
        const DealStatusRequest &request) const {
      auto maybe_state = handleDealStatus(request);
      ProviderDealState state;
      if (maybe_state.has_error()) {
        state.status = StorageDealStatus::STORAGE_DEAL_ERROR;
        state.message = maybe_state.error().message();
      } else {
        state = std::move(maybe_state.value());
      }
      DealStatusResponseType response{state};
      OUTCOME_TRY(digest, response.getDigest());
      OUTCOME_TRYA(response.signature, sign(digest));
      return response;
    }

    /**
     * Handles deal status request.
     * Checks request signature and looks for deal state.
     * @param request - contains proposal CID
     * @return deal state for requested deal
     */
    outcome::result<MinerDeal> handleDealStatus(
        const DealStatusRequest &request) const;

    /**
     * Sets deal Mk protocol handler for different protocol versions.
     */
    template <typename ProposalType>
    inline void setDealMkHandler(const std::string &protocol) {
      host_->setProtocolHandler(
          protocol, [self_wptr{weak_from_this()}, protocol](auto stream) {
            auto cbor_stream{
                std::make_shared<common::libp2p::CborStream>(stream)};
            if (auto self = self_wptr.lock()) {
              self->logger_->debug("New deal stream");
              cbor_stream->template read<ProposalType>(
                  [self, cbor_stream, protocol](
                      const outcome::result<ProposalType> &proposal) {
                    if (proposal.has_error()) {
                      self->logger_->error("Read proposal error:"
                                           + proposal.error().message());
                      closeStreamGracefully(cbor_stream, self->logger_);
                    }
                    self->handleMKDealStream(
                        protocol, cbor_stream, proposal.value());
                  });
            }
          });
    }

    /**
     * Handle incoming deal proposal stream
     * @param protocol - what protocol is used to communicate to client
     * @param stream - stream to client
     * @param proposal - client deal proposal
     */
    void handleMKDealStream(const std::string &protocol,
                            const std::shared_ptr<CborStream> &stream,
                            const Proposal &proposal);

    /**
     * Verify client signature for deal proposal
     * @param deal to verify
     * @return true if verified or false otherwise
     */
    outcome::result<bool> verifyDealProposal(
        const std::shared_ptr<MinerDeal> &deal) const;

    /**
     * Ensure provider has enough funds
     * @param deal - storage deal
     * @return cid of funding message if it was sent
     */
    outcome::result<boost::optional<CID>> ensureProviderFunds(
        const std::shared_ptr<MinerDeal> &deal);

    /**
     * Publish storage deal
     * @param deal to publish
     * @return CID of message sent
     */
    outcome::result<CID> publishDeal(const std::shared_ptr<MinerDeal> &deal);

    /**
     * Send signed response to storage deal proposal and close connection
     * @param deal_context
     */
    outcome::result<void> sendSignedResponse(
        const std::shared_ptr<DealContext> &deal_context);

    /**
     * Locate piece for deal
     * @param deal - activated deal
     * @return piece info with location
     */
    outcome::result<PieceLocation> locatePiece(
        const std::shared_ptr<MinerDeal> &deal);

    /**
     * Records sector information about an activated deal so that the data can
     * be retrieved later
     * @param deal - activated deal
     * @param piece_location - piece location
     * @return error in case of failure
     */
    outcome::result<void> recordPieceInfo(
        const std::shared_ptr<MinerDeal> &deal,
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
    outcome::result<void> finalizeDeal(const std::shared_ptr<MinerDeal> &deal);

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
    FSM_HANDLE_DEFINITION(onProviderEventOpen);

    /**
     * @brief Handle event deal accepted
     * @param deal  - current storage deal
     * @param event - ProviderEventDealAccepted
     * @param from  - STORAGE_DEAL_VALIDATING
     * @param to    - STORAGE_DEAL_PROPOSAL_ACCEPTED
     */
    FSM_HANDLE_DEFINITION(onProviderEventDealAccepted);

    /**
     * @brief Handle event waiting for manual data
     * @param deal  - current storage deal
     * @param event - ProviderEventWaitingForManualData
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
     * @param to    - STORAGE_DEAL_WAITING_FOR_DATA
     */
    FSM_HANDLE_DEFINITION(onProviderEventWaitingForManualData);

    /**
     * @brief Handle event funding initiated
     * @param deal  - current storage deal
     * @param event - ProviderEventWaitingForManualData
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
     * @param to    - STORAGE_DEAL_WAITING_FOR_DATA
     */
    FSM_HANDLE_DEFINITION(onProviderEventFundingInitiated);

    /**
     * @brief Handle event funded
     * @param deal  - current storage deal
     * @param event - ProviderEventWaitingForManualData
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
     * @param to    - STORAGE_DEAL_WAITING_FOR_DATA
     */
    FSM_HANDLE_DEFINITION(onProviderEventFunded);

    /**
     * @brief Handle event data transfer initiated
     * @param deal  - current storage deal
     * @param event - ProviderEventDataTransferInitiated
     * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
     * @param to    - STORAGE_DEAL_TRANSFERRING
     */
    FSM_HANDLE_DEFINITION(onProviderEventDataTransferInitiated);

    /**
     * @brief Handle event data transfer completed
     * @param deal  - current storage deal
     * @param event - ProviderEventDataTransferCompleted
     * @param from  - STORAGE_DEAL_TRANSFERRING
     * @param to    - STORAGE_DEAL_VERIFY_DATA
     */
    FSM_HANDLE_DEFINITION(onProviderEventDataTransferCompleted);

    /**
     * @brief Handle event data verified
     * @param deal  - current storage deal
     * @param event - ProviderEventVerifiedData
     * @param from  - STORAGE_DEAL_VERIFY_DATA
     * @param to    - STORAGE_DEAL_FAILING
     */
    FSM_HANDLE_DEFINITION(onProviderEventVerifiedData);

    /**
     * @brief Handle event deal publish initiated
     * @param deal  - current storage deal
     * @param event - ProviderEventDealPublishInitiated
     * @param from  - STORAGE_DEAL_VERIFY_DATA
     * @param to    - STORAGE_DEAL_FAILING
     */
    FSM_HANDLE_DEFINITION(onProviderEventDealPublishInitiated);

    /**
     * @brief Handle event deal published
     * @param deal  - current storage deal
     * @param event - ProviderEventDealPublished
     * @param from  - STORAGE_DEAL_VERIFY_DATA or STORAGE_DEAL_WAITING_FOR_DATA
     * @param to    - STORAGE_DEAL_ENSURE_PROVIDER_FUNDS
     */
    FSM_HANDLE_DEFINITION(onProviderEventDealPublished);

    /**
     * @brief Handle event handoff
     * @param deal  - current storage deal
     * @param event - ProviderEventDealHandedOff
     * @param from  - STORAGE_DEAL_STAGED
     * @param to    - STORAGE_DEAL_SEALING
     */
    FSM_HANDLE_DEFINITION(onProviderEventDealHandedOff);

    /**
     * @brief Handle event deal activation
     * @param deal  - current storage deal
     * @param event - ProviderEventDealActivated
     * @param from  - STORAGE_DEAL_SEALING
     * @param to    - STORAGE_DEAL_ACTIVE
     */
    FSM_HANDLE_DEFINITION(onProviderEventDealActivated);

    /**
     * @brief Handle event deal completed
     * @param deal  - current storage deal
     * @param event - ProviderEventDealCompleted
     * @param from  - STORAGE_DEAL_ACTIVE
     * @param to    - STORAGE_DEAL_FAILING
     */
    FSM_HANDLE_DEFINITION(onProviderEventDealCompleted);

    /**
     * @brief Handle event failed
     * @param deal  - current storage deal
     * @param event - ProviderEventFailed
     * @param from  - STORAGE_DEAL_FAILING
     * @param to    - STORAGE_DEAL_ERROR
     */
    FSM_HANDLE_DEFINITION(onProviderEventFailed);

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

    // coonnection manager
    std::mutex connections_mutex_;
    // TODO(turuslan): FIL-420 check cache memory usage
    std::map<CID, std::shared_ptr<CborStream>> connections_;

    /** State machine */
    std::shared_ptr<ProviderFSM> fsm_;

    std::shared_ptr<Host> host_;
    std::shared_ptr<boost::asio::io_context> context_;
    std::shared_ptr<StoredAsk> stored_ask_;
    std::shared_ptr<FullNodeApi> api_;
    std::shared_ptr<SectorBlocks> sector_blocks_;
    std::shared_ptr<ChainEvents> chain_events_;
    Address miner_actor_address_;
    std::shared_ptr<PieceIO> piece_io_;
    std::shared_ptr<PieceStorage> piece_storage_;
    std::shared_ptr<FileStore> filestore_;
    IpldPtr ipld_;
    std::shared_ptr<DataTransfer> datatransfer_;
    std::shared_ptr<DealInfoManager> deal_info_manager_;

    common::Logger logger_ = common::createLogger("StorageMarketProvider");
  };

  void serveDealStatus(libp2p::Host &host,
                       std::weak_ptr<StorageProviderImpl> _provider);

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
