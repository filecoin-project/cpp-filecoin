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
#include "storage/piece/impl/piece_storage_impl.hpp"
#include "vm/actor/builtin/market/actor.hpp"

#define CALLBACK_ACTION(_action)                                          \
  [self{shared_from_this()}](auto deal, auto event, auto from, auto to) { \
    self->logger_->debug("Provider FSM " #_action);                       \
    self->_action(deal, event, from, to);                                 \
    deal->state = to;                                                     \
  }

namespace fc::markets::storage::provider {

  using api::MsgWait;
  using fc::storage::piece::PieceStorageImpl;
  using host::HostContext;
  using host::HostContextImpl;
  using vm::VMExitCode;
  using vm::actor::MethodParams;
  using vm::actor::builtin::market::getProposalCid;
  using vm::actor::builtin::market::PublishStorageDeals;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  StorageProviderImpl::StorageProviderImpl(
      const RegisteredProof &registered_proof,
      std::shared_ptr<Host> host,
      std::shared_ptr<boost::asio::io_context> context,
      std::shared_ptr<KeyStore> keystore,
      std::shared_ptr<Datastore> datastore,
      std::shared_ptr<Api> api,
      std::shared_ptr<MinerApi> miner_api,
      const Address &actor_address,
      std::shared_ptr<PieceIO> piece_io)
      : registered_proof_{registered_proof},
        host_{std::move(host)},
        context_{std::move(context)},
        stored_ask_{std::make_shared<StoredAsk>(
            keystore, datastore, api, actor_address)},
        api_{std::move(api)},
        miner_api_{std::move(miner_api)},
        network_{std::make_shared<Libp2pStorageMarketNetwork>(host_)},
        piece_io_{std::move(piece_io)},
        piece_storage_{std::make_shared<PieceStorageImpl>(datastore)} {}

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

  outcome::result<std::shared_ptr<MinerDeal>> StorageProviderImpl::getDeal(
      const CID &proposal_cid) const {
    auto it = local_deals_.find(proposal_cid);
    if (it == local_deals_.end()) {
      return StorageMarketProviderError::LOCAL_DEAL_NOT_FOUND;
    }
    return it->second;
  }

  outcome::result<void> StorageProviderImpl::addStorageCollateral(
      const TokenAmount &amount) {
    return TodoError::ERROR;
  }

  outcome::result<TokenAmount> StorageProviderImpl::getStorageCollateral() {
    return outcome::failure(TodoError::ERROR);
  }

  outcome::result<void> StorageProviderImpl::importDataForDeal(
      const CID &proposal_cid, const Buffer &data) {
    OUTCOME_TRY(piece_commitment,
                piece_io_->generatePieceCommitment(registered_proof_, data));
    OUTCOME_TRY(deal, getDeal(proposal_cid));
    if (piece_commitment.first
        != deal->client_deal_proposal.proposal.piece_cid) {
      return StorageMarketProviderError::PIECE_CID_DOESNT_MATCH;
    }

    OUTCOME_TRY(fsm_->send(deal, ProviderEvent::ProviderEventVerifiedData));
    return outcome::success();
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
      if (proposal.has_error()) {
        self->logger_->error("Read proposal error");
        self->network_->closeStreamGracefully(stream);
        return;
      }

      auto proposal_cid = getProposalCid(proposal.value().deal_proposal);
      if (proposal_cid.has_error()) {
        self->logger_->error("Read proposal error");
        self->network_->closeStreamGracefully(stream);
        return;
      }
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
                    .proposal_cid = proposal_cid.value(),
                    .add_funds_cid = {},
                    .publish_cid = {},
                    .miner = self->host_->getPeerInfo(),
                    .client = remote_peer_info,
                    .state = StorageDealStatus::STORAGE_DEAL_UNKNOWN,
                    .piece_path = {},
                    .metadata_path = {},
                    .connection_closed = false,
                    .message = {},
                    .ref = proposal.value().piece,
                    .deal_id = {}});
      self->local_deals_[proposal_cid.value()] = deal;
      self->connections_[proposal_cid.value()] = stream;
      OUTCOME_EXCEPT(
          self->fsm_->begin(deal, StorageDealStatus::STORAGE_DEAL_UNKNOWN));
      OUTCOME_EXCEPT(self->fsm_->send(deal, ProviderEvent::ProviderEventOpen));
    });
  }

  outcome::result<boost::optional<CID>>
  StorageProviderImpl::ensureProviderFunds(std::shared_ptr<MinerDeal> deal) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(worker_info,
                api_->StateMinerInfo(
                    deal->client_deal_proposal.proposal.provider, tipset_key));
    OUTCOME_TRY(maybe_cid,
                api_->MarketEnsureAvailable(
                    deal->client_deal_proposal.proposal.provider,
                    worker_info.worker,
                    deal->client_deal_proposal.proposal.provider_collateral,
                    tipset_key));
    return std::move(maybe_cid);
  }

  outcome::result<CID> StorageProviderImpl::publishDeal(
      std::shared_ptr<MinerDeal> deal) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(worker_info,
                api_->StateMinerInfo(
                    deal->client_deal_proposal.proposal.provider, tipset_key));
    std::vector<ClientDealProposal> params{deal->client_deal_proposal};
    OUTCOME_TRY(encoded_params, codec::cbor::encode(params));
    UnsignedMessage unsigned_message(vm::actor::kStorageMarketAddress,
                                     worker_info.worker,
                                     0,
                                     TokenAmount{0},
                                     kGasPrice,
                                     kGasLimit,
                                     PublishStorageDeals::Number,
                                     MethodParams{encoded_params});
    OUTCOME_TRY(signed_message, api_->MpoolPushMessage(unsigned_message));
    OUTCOME_TRY(cid, signed_message.getCid());
    OUTCOME_TRY(str_cid, cid.toString());
    logger_->debug("Deal published with CID = " + str_cid);
    return std::move(cid);
  }

  void StorageProviderImpl::sendSignedResponse(std::shared_ptr<MinerDeal> deal,
                                               const StorageDealStatus &status,
                                               const std::string &message) {
    Response response{.state = status,
                      .message = message,
                      .proposal = deal->proposal_cid,
                      .publish_message = deal->publish_cid};
    // TODO handle if stream is absent
    auto stream = connections_[deal->proposal_cid];
    stream->write(
        response,
        [self{shared_from_this()}, stream, deal](
            outcome::result<size_t> maybe_res) {
          if (!self->hasValue(
                  maybe_res, "Write deal response error ", stream)) {
            OUTCOME_EXCEPT(self->fsm_->send(
                deal, ProviderEvent::ProviderEventSendResponseFailed));
            return;
          }
          self->network_->closeStreamGracefully(stream);
          self->logger_->debug("Deal response written, connection closed");
          OUTCOME_EXCEPT(self->fsm_->send(
              deal, ProviderEvent::ProviderEventDealPublished));
        });
  }

  outcome::result<void> StorageProviderImpl::recordPieceInfo(
      std::shared_ptr<MinerDeal> deal) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());

    // miner_node_api.LocatePieceForDealWithinSector()
    // TODO PieceStorage.addPayloadLocations
    // TODO PieceStorage.addPieceInfo

    return outcome::success();
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
    if (deal->ref.transfer_type == kTransferTypeManual) {
      OUTCOME_EXCEPT(
          fsm_->send(deal, ProviderEvent::ProviderEventWaitingForManualData));
      return;
    }

    // TODO transfer data
  }

  void StorageProviderImpl::onProviderEventWaitingForManualData(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // wait for importDataForDeal() call
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
    auto maybe_cid = publishDeal(deal);
    if (maybe_cid.has_error()) {
      OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventNodeErrored));
      return;
    }

    deal->publish_cid = maybe_cid.value();
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
    auto maybe_cid = ensureProviderFunds(deal);
    if (maybe_cid.has_error()) {
      OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventNodeErrored));
      return;
    }

    // funding message was sent
    if (maybe_cid.value().has_value()) {
      deal->add_funds_cid = *maybe_cid.value();
      OUTCOME_EXCEPT(
          fsm_->send(deal, ProviderEvent::ProviderEventFundingInitiated));
      return;
    }

    OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventFunded));
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
    auto maybe_wait = api_->StateWaitMsg(deal->publish_cid);
    if (maybe_wait.has_error()) {
      OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventNodeErrored));
      return;
    }
    maybe_wait.value().wait(
        [self{shared_from_this()}, deal](outcome::result<MsgWait> result) {
          if (result.has_error()) {
            self->logger_->error("Publish storage deal message error "
                                 + result.error().message());
            OUTCOME_EXCEPT(self->fsm_->send(
                deal, ProviderEvent::ProviderEventDealPublishError));
            return;
          }
          if (result.value().receipt.exit_code != VMExitCode::Ok) {
            self->logger_->error("Publish storage deal exit code "
                                 + std::to_string(static_cast<uint64_t>(
                                     result.value().receipt.exit_code)));
            OUTCOME_EXCEPT(self->fsm_->send(
                deal, ProviderEvent::ProviderEventDealPublishError));
            return;
          }
          auto maybe_res = codec::cbor::decode<PublishStorageDeals::Result>(
              result.value().receipt.return_value);
          if (maybe_res.has_error()) {
            self->logger_->error("Publish storage deal decode result error");
            OUTCOME_EXCEPT(self->fsm_->send(
                deal, ProviderEvent::ProviderEventDealPublishError));
            return;
          }
          if (maybe_res.value().deals.size() != 1) {
            self->logger_->error("Publish storage deal result size error");
            OUTCOME_EXCEPT(self->fsm_->send(
                deal, ProviderEvent::ProviderEventDealPublishError));
            return;
          }
          deal->deal_id = maybe_res.value().deals.front();
          self->sendSignedResponse(
              deal, StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED, "");
        });
  }

  void StorageProviderImpl::onProviderEventDealPublished(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO hand off
    // miner_node_api.addPiece
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
    // on deal sector commited
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
    auto res = recordPieceInfo(deal);
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

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::provider,
                            StorageMarketProviderError,
                            e) {
  using fc::markets::storage::provider::StorageMarketProviderError;

  switch (e) {
    case StorageMarketProviderError::LOCAL_DEAL_NOT_FOUND:
      return "StorageMarketProviderError: local deal not found";
    case StorageMarketProviderError::PIECE_CID_DOESNT_MATCH:
      return "StorageMarketProviderError: imported piece cid doensn't match "
             "proposal piece cid";
  }

  return "StorageMarketProviderError: unknown error";
}
