/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/provider.hpp"

#include "common/libp2p/peer/peer_info_helper.hpp"
#include "common/todo_error.hpp"
#include "host/context/host_context.hpp"
#include "host/context/impl/host_context_impl.hpp"
#include "markets/storage/provider/storage_provider_error.hpp"
#include "markets/storage/provider/stored_ask.hpp"

#define CALLBACK_ACTION(_action)                                          \
  [self{shared_from_this()}](auto deal, auto event, auto from, auto to) { \
    self->logger_->debug("Provider FSM " #_action);                       \
    self->_action(deal, event, from, to);                                 \
    deal->state = to;                                                     \
  }

namespace fc::markets::storage::provider {

  using host::HostContext;
  using host::HostContextImpl;

  StorageProviderImpl::StorageProviderImpl(
      std::shared_ptr<Host> host,
      std::shared_ptr<boost::asio::io_context> context,
      std::shared_ptr<KeyStore> keystore,
      std::shared_ptr<Datastore> datastore,
      std::shared_ptr<Api> api,
      const Address &actor_address)
      : host_{std::move(host)},
        context_{std::move(context)},
        stored_ask_{std::make_shared<StoredAsk>(
            keystore, datastore, api, actor_address)},
        network_{std::make_shared<Libp2pStorageMarketNetwork>(host_)} {}

  void StorageProviderImpl::init() {
    std::shared_ptr<HostContext> fsm_context =
        std::make_shared<HostContextImpl>(context_);
    fsm_ = std::make_shared<ProviderFSM>(makeFSMTransitions(), fsm_context);
  }

  outcome::result<void> StorageProviderImpl::start() {
    OUTCOME_TRY(network_->setDelegate(shared_from_this()));

    context_->post([self{shared_from_this()}] {
      self->host_->start();
      self->logger_->debug(
          "Server started\nListening on: "
          + peerInfoToPrettyString(self->host_->getPeerInfo()));
    });

    return outcome::success();
  }

  outcome::result<void> StorageProviderImpl::addAsk(const TokenAmount &price,
                                                    ChainEpoch duration) {
    return stored_ask_->addAsk(price, duration);
  }

  outcome::result<std::vector<SignedStorageAsk>> StorageProviderImpl::listAsks(
      const Address &address) {
    std::vector<SignedStorageAsk> result;
    OUTCOME_TRY(signed_storage_ask, stored_ask_->getAsk(address));
    result.push_back(signed_storage_ask);
    return result;
  }

  outcome::result<std::vector<StorageDeal>> StorageProviderImpl::listDeals() {
    return TodoError::ERROR;
  }

  outcome::result<std::vector<MinerDeal>>
  StorageProviderImpl::listIncompleteDeals() {
    return TodoError::ERROR;
  }

  outcome::result<void> StorageProviderImpl::addStorageCollateral(
      const TokenAmount &amount) {
    return TodoError::ERROR;
  }

  outcome::result<TokenAmount> StorageProviderImpl::getStorageCollateral() {
    return outcome::failure(TodoError::ERROR);
  }

  outcome::result<void> StorageProviderImpl::importDataForDeal(
      const CID &prop_cid, const libp2p::connection::Stream &data) {
    return TodoError::ERROR;
  }

  void StorageProviderImpl::handleAskStream(
      const std::shared_ptr<CborStream> &stream) {
    logger_->debug("New ask stream");
    stream->read<AskRequest>([self{shared_from_this()},
                              stream](outcome::result<AskRequest> request_res) {
      if (!self->hasValue(request_res, "Ask request error ", stream)) return;
      auto maybe_ask = self->stored_ask_->getAsk(request_res.value().miner);
      if (!self->hasValue(maybe_ask, "Get stored ask error ", stream)) return;
      AskResponse response{.ask = maybe_ask.value()};
      stream->write(
          response, [self, stream](outcome::result<size_t> maybe_res) {
            if (!self->hasValue(maybe_res, "Write ask response error ", stream))
              return;
            self->network_->closeStreamGracefully(stream);
            self->logger_->debug("Ask response written, connection closed");
          });
    });
  }

  void StorageProviderImpl::handleDealStream(
      const std::shared_ptr<CborStream> &stream) {
    logger_->debug("New deal stream");

    stream->read<Proposal>([self{shared_from_this()},
                            stream](outcome::result<Proposal> proposal) {
      // todo check result
      if (proposal.has_error()) {
        self->logger_->error("Read proposal error");
        self->network_->closeStreamGracefully(stream);
        return;
      }

      // todo cid from proposal
      // TODO wrong proposal cid
      CID proposal_cid = proposal.value().deal_proposal.proposal.piece_cid;
      auto remote_peer_id = stream->stream()->remotePeerId();
      auto remote_multiaddress = stream->stream()->remoteMultiaddr();
      if (remote_peer_id.has_error() || remote_multiaddress.has_error()) {
        self->logger_->error("Cannot get remote peer info");
        self->network_->closeStreamGracefully(stream);
        return;
      }

      PeerInfo remote_peer_info{.id = remote_peer_id.value(),
                                .addresses = {remote_multiaddress.value()}};
      std::shared_ptr<MinerDeal> deal = std::make_shared<MinerDeal>(
          MinerDeal{.client_deal_proposal = proposal.value().deal_proposal,
                    .proposal_cid = proposal_cid,
                    .miner = self->host_->getPeerInfo(),
                    .client = remote_peer_info,
                    .state = StorageDealStatus::STORAGE_DEAL_UNKNOWN,
                    .piece_path = {},
                    .metadata_path = {},
                    .connection_closed = false,
                    .message = {},
                    .ref = proposal.value().piece,
                    .deal_id = {}});
      self->local_deals_[proposal_cid] = deal;
      self->connections_[proposal_cid] = stream;
      OUTCOME_EXCEPT(
          self->fsm_->begin(deal, StorageDealStatus::STORAGE_DEAL_UNKNOWN));
      OUTCOME_EXCEPT(self->fsm_->send(deal, ProviderEvent::ProviderEventOpen));
    });
  }

  std::vector<ProviderTransition> StorageProviderImpl::makeFSMTransitions() {
    return {
        ProviderTransition(ProviderEvent::ProviderEventOpen)
            .from(StorageDealStatus::STORAGE_DEAL_UNKNOWN)
            .to(StorageDealStatus::STORAGE_DEAL_VALIDATING)
            .action(CALLBACK_ACTION(onProviderEventOpen)),
        ProviderTransition(ProviderEvent::ProviderEventNodeErrored)
            .fromAny()
            .to(StorageDealStatus::STORAGE_DEAL_FAILING)
            .action(CALLBACK_ACTION(onProviderEventNodeErrored)),
        ProviderTransition(ProviderEvent::ProviderEventDealRejected)
            .fromMany(StorageDealStatus::STORAGE_DEAL_VALIDATING,
                      StorageDealStatus::STORAGE_DEAL_VERIFY_DATA)
            .to(StorageDealStatus::STORAGE_DEAL_FAILING)
            .action(CALLBACK_ACTION(onProviderEventDealRejected)),
        ProviderTransition(ProviderEvent::ProviderEventDealAccepted)
            .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
            .to(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
            .action(CALLBACK_ACTION(onProviderEventDealAccepted)),
        ProviderTransition(ProviderEvent::ProviderEventWaitingForManualData)
            .from(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
            .to(StorageDealStatus::STORAGE_DEAL_WAITING_FOR_DATA)
            .action(CALLBACK_ACTION(onProviderEventWaitingForManualData)),
        ProviderTransition(ProviderEvent::ProviderEventDataTransferFailed)
            .fromMany(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED,
                      StorageDealStatus::STORAGE_DEAL_TRANSFERRING)
            .to(StorageDealStatus::STORAGE_DEAL_FAILING)
            .action(CALLBACK_ACTION(onProviderEventDataTransferFailed)),
        ProviderTransition(ProviderEvent::ProviderEventDataTransferInitiated)
            .from(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
            .to(StorageDealStatus::STORAGE_DEAL_TRANSFERRING)
            .action(CALLBACK_ACTION(onProviderEventDataTransferInitiated)),
        ProviderTransition(ProviderEvent::ProviderEventDataTransferCompleted)
            .from(StorageDealStatus::STORAGE_DEAL_TRANSFERRING)
            .to(StorageDealStatus::STORAGE_DEAL_VERIFY_DATA)
            .action(CALLBACK_ACTION(onProviderEventDataTransferCompleted)),
        ProviderTransition(ProviderEvent::ProviderEventGeneratePieceCIDFailed)
            .from(StorageDealStatus::STORAGE_DEAL_VERIFY_DATA)
            .to(StorageDealStatus::STORAGE_DEAL_FAILING)
            .action(CALLBACK_ACTION(onProviderEventGeneratePieceCIDFailed)),
        ProviderTransition(ProviderEvent::ProviderEventVerifiedData)
            .fromMany(StorageDealStatus::STORAGE_DEAL_VERIFY_DATA,
                      StorageDealStatus::STORAGE_DEAL_WAITING_FOR_DATA)
            .to(StorageDealStatus::STORAGE_DEAL_ENSURE_PROVIDER_FUNDS)
            .action(CALLBACK_ACTION(onProviderEventVerifiedData)),
        ProviderTransition(ProviderEvent::ProviderEventFundingInitiated)
            .from(StorageDealStatus::STORAGE_DEAL_ENSURE_PROVIDER_FUNDS)
            .to(StorageDealStatus::STORAGE_DEAL_PROVIDER_FUNDING)
            .action(CALLBACK_ACTION(onProviderEventFundingInitiated)),
        ProviderTransition(ProviderEvent::ProviderEventFunded)
            .fromMany(StorageDealStatus::STORAGE_DEAL_PROVIDER_FUNDING,
                      StorageDealStatus::STORAGE_DEAL_ENSURE_PROVIDER_FUNDS)
            .to(StorageDealStatus::STORAGE_DEAL_PUBLISH)
            .action(CALLBACK_ACTION(onProviderEventFunded)),
        ProviderTransition(ProviderEvent::ProviderEventDealPublishInitiated)
            .from(StorageDealStatus::STORAGE_DEAL_PUBLISH)
            .to(StorageDealStatus::STORAGE_DEAL_PUBLISHING)
            .action(CALLBACK_ACTION(onProviderEventDealPublishInitiated)),
        ProviderTransition(ProviderEvent::ProviderEventDealPublishError)
            .from(StorageDealStatus::STORAGE_DEAL_PUBLISHING)
            .to(StorageDealStatus::STORAGE_DEAL_FAILING)
            .action(CALLBACK_ACTION(onProviderEventDealPublishError)),
        ProviderTransition(ProviderEvent::ProviderEventSendResponseFailed)
            .fromMany(StorageDealStatus::STORAGE_DEAL_PUBLISHING,
                      StorageDealStatus::STORAGE_DEAL_FAILING)
            .to(StorageDealStatus::STORAGE_DEAL_ERROR)
            .action(CALLBACK_ACTION(onProviderEventSendResponseFailed)),
        ProviderTransition(ProviderEvent::ProviderEventDealPublished)
            .from(StorageDealStatus::STORAGE_DEAL_PUBLISHING)
            .to(StorageDealStatus::STORAGE_DEAL_STAGED)
            .action(CALLBACK_ACTION(onProviderEventDealPublished)),
        ProviderTransition(ProviderEvent::ProviderEventFileStoreErrored)
            .fromMany(StorageDealStatus::STORAGE_DEAL_STAGED,
                      StorageDealStatus::STORAGE_DEAL_SEALING,
                      StorageDealStatus::STORAGE_DEAL_ACTIVE)
            .to(StorageDealStatus::STORAGE_DEAL_FAILING)
            .action(CALLBACK_ACTION(onProviderEventFileStoreErrored)),
        ProviderTransition(ProviderEvent::ProviderEventDealHandoffFailed)
            .from(StorageDealStatus::STORAGE_DEAL_STAGED)
            .to(StorageDealStatus::STORAGE_DEAL_FAILING)
            .action(CALLBACK_ACTION(onProviderEventDealHandoffFailed)),
        ProviderTransition(ProviderEvent::ProviderEventDealHandedOff)
            .from(StorageDealStatus::STORAGE_DEAL_STAGED)
            .to(StorageDealStatus::STORAGE_DEAL_SEALING)
            .action(CALLBACK_ACTION(onProviderEventDealHandedOff)),
        ProviderTransition(ProviderEvent::ProviderEventDealActivationFailed)
            .from(StorageDealStatus::STORAGE_DEAL_SEALING)
            .to(StorageDealStatus::STORAGE_DEAL_FAILING)
            .action(CALLBACK_ACTION(onProviderEventDealActivationFailed)),
        ProviderTransition(ProviderEvent::ProviderEventDealActivated)
            .from(StorageDealStatus::STORAGE_DEAL_SEALING)
            .to(StorageDealStatus::STORAGE_DEAL_ACTIVE)
            .action(CALLBACK_ACTION(onProviderEventDealActivated)),
        ProviderTransition(ProviderEvent::ProviderEventPieceStoreErrored)
            .from(StorageDealStatus::STORAGE_DEAL_ACTIVE)
            .to(StorageDealStatus::STORAGE_DEAL_FAILING)
            .action(CALLBACK_ACTION(onProviderEventPieceStoreErrored)),
        ProviderTransition(ProviderEvent::ProviderEventDealCompleted)
            .from(StorageDealStatus::STORAGE_DEAL_ACTIVE)
            .to(StorageDealStatus::STORAGE_DEAL_COMPLETED)
            .action(CALLBACK_ACTION(onProviderEventDealCompleted)),
        ProviderTransition(ProviderEvent::ProviderEventUnableToLocatePiece)
            .from(StorageDealStatus::STORAGE_DEAL_ACTIVE)
            .to(StorageDealStatus::STORAGE_DEAL_FAILING)
            .action(CALLBACK_ACTION(onProviderEventUnableToLocatePiece)),
        ProviderTransition(ProviderEvent::ProviderEventReadMetadataErrored)
            .from(StorageDealStatus::STORAGE_DEAL_ACTIVE)
            .to(StorageDealStatus::STORAGE_DEAL_FAILING)
            .action(CALLBACK_ACTION(onProviderEventReadMetadataErrored)),
        ProviderTransition(ProviderEvent::ProviderEventFailed)
            .from(StorageDealStatus::STORAGE_DEAL_FAILING)
            .to(StorageDealStatus::STORAGE_DEAL_ERROR)
            .action(CALLBACK_ACTION(onProviderEventFailed))};
  }

  void StorageProviderImpl::onProviderEventOpen(std::shared_ptr<MinerDeal> deal,
                                                ProviderEvent event,
                                                StorageDealStatus from,
                                                StorageDealStatus to) {
    // TODO validate deal proposal
    OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventDealAccepted));
  }

  void StorageProviderImpl::onProviderEventNodeErrored(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventDealRejected(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventDealAccepted(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO transfer data
    OUTCOME_EXCEPT(
        fsm_->send(deal, ProviderEvent::ProviderEventWaitingForManualData));
  }

  void StorageProviderImpl::onProviderEventWaitingForManualData(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO
    OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventVerifiedData));
  }

  void StorageProviderImpl::onProviderEventInsufficientFunds(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventFundingInitiated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO WaitForFunding
    OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventFunded));
  }

  void StorageProviderImpl::onProviderEventFunded(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO PublishDeal
    OUTCOME_EXCEPT(
        fsm_->send(deal, ProviderEvent::ProviderEventDealPublishInitiated));
  }

  void StorageProviderImpl::onProviderEventDataTransferFailed(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventDataTransferInitiated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventDataTransferCompleted(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventManualDataReceived(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventGeneratePieceCIDFailed(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventVerifiedData(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO EnsureProviderFunds
    OUTCOME_EXCEPT(
        fsm_->send(deal, ProviderEvent::ProviderEventFundingInitiated));
  }

  void StorageProviderImpl::onProviderEventSendResponseFailed(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventDealPublishInitiated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO WaitForPublish
    OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventDealPublished));
  }

  void StorageProviderImpl::onProviderEventDealPublished(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO hand off
    OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventDealHandedOff));
  }

  void StorageProviderImpl::onProviderEventDealPublishError(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventFileStoreErrored(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventDealHandoffFailed(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventDealHandedOff(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO verify deal activated
    OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventDealActivated));
  }

  void StorageProviderImpl::onProviderEventDealActivationFailed(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventUnableToLocatePiece(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventDealActivated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO Record Piece Info
    OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventDealCompleted));
  }

  void StorageProviderImpl::onProviderEventPieceStoreErrored(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventReadMetadataErrored(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

  void StorageProviderImpl::onProviderEventDealCompleted(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    logger_->debug("Deal completed");
  }

  void StorageProviderImpl::onProviderEventFailed(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {}

}  // namespace fc::markets::storage::provider
