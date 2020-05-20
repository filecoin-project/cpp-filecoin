/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "provider_impl.hpp"

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
  using fc::storage::piece::PayloadLocation;
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
      const Address &miner_actor_address,
      std::shared_ptr<PieceIO> piece_io)
      : registered_proof_{registered_proof},
        host_{std::move(host)},
        context_{std::move(context)},
        keystore_{std::move(keystore)},
        stored_ask_{
            std::make_shared<StoredAsk>(datastore, api, miner_actor_address)},
        api_{std::move(api)},
        miner_api_{std::move(miner_api)},
        miner_actor_address_{miner_actor_address},
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
      if (!self->hasValue(proposal, "Read proposal error: ", stream)) return;
      auto proposal_cid = getProposalCid(proposal.value().deal_proposal);
      if (!self->hasValue(proposal, "Read proposal error: ", stream)) return;

      auto remote_peer_id = stream->stream()->remotePeerId();
      if (!self->hasValue(
              remote_peer_id, "Cannot get remote peer info: ", stream))
        return;
      auto remote_multiaddress = stream->stream()->remoteMultiaddr();
      if (!self->hasValue(
              remote_multiaddress, "Cannot get remote peer info: ", stream))
        return;
      PeerInfo remote_peer_info{.id = remote_peer_id.value(),
                                .addresses = {remote_multiaddress.value()}};
      std::shared_ptr<MinerDeal> deal = std::make_shared<MinerDeal>(
          MinerDeal{.client_deal_proposal = proposal.value().deal_proposal,
                    .proposal_cid = proposal_cid.value(),
                    .add_funds_cid = boost::none,
                    .publish_cid = boost::none,
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

  outcome::result<bool> StorageProviderImpl::verifyDealProposal(
      std::shared_ptr<MinerDeal> deal) const {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    auto proposal = deal->client_deal_proposal.proposal;
    OUTCOME_TRY(client_key_address,
                api_->StateAccountKey(proposal.client, tipset_key));
    OUTCOME_TRY(proposal_bytes, codec::cbor::encode(proposal));
    OUTCOME_TRY(verified,
                keystore_->verify(client_key_address,
                                  proposal_bytes,
                                  deal->client_deal_proposal.client_signature));
    if (!verified) {
      deal->message = "Deal proposal verification failed, wrong signature";
      return false;
    }

    if (proposal.provider != miner_actor_address_) {
      deal->message =
          "Deal proposal verification failed, incorrect provider for deal";
      return false;
    }

    if (static_cast<ChainEpoch>(chain_head.height)
        > proposal.start_epoch - kDefaultDealAcceptanceBuffer) {
      deal->message =
          "Deal proposal verification failed, deal start epoch is too soon or "
          "deal already expired";
      return false;
    }

    OUTCOME_TRY(ask, stored_ask_->getAsk(miner_actor_address_));
    auto min_price =
        (ask.ask.price * static_cast<uint64_t>(proposal.piece_size))
        / (1 << 30);
    if (proposal.storage_price_per_epoch < min_price) {
      std::stringstream ss;
      ss << "Deal proposal verification failed, storage price per epoch less "
            "than asking price: "
         << proposal.storage_price_per_epoch << " < " << min_price;
      deal->message = ss.str();
      return false;
    }

    if (proposal.piece_size < ask.ask.min_piece_size) {
      std::stringstream ss;
      ss << "Deal proposal verification failed, piece size less than minimum "
            "required size: "
         << proposal.piece_size << " < " << ask.ask.min_piece_size;
      deal->message = ss.str();
      return false;
    }
    if (proposal.piece_size > ask.ask.max_piece_size) {
      std::stringstream ss;
      ss << "Deal proposal verification failed, piece size more than maximum "
            "allowed size: "
         << proposal.piece_size << " > " << ask.ask.max_piece_size;
      deal->message = ss.str();
      return false;
    }

    // This doesn't guarantee that the client won't withdraw / lock those funds
    // but it's a decent first filter
    OUTCOME_TRY(client_balance,
                api_->StateMarketBalance(proposal.client, tipset_key));
    if (client_balance.available < proposal.getTotalStorageFee()) {
      std::stringstream ss;
      ss << "Deal proposal verification failed, client market available "
            "balance too small: "
         << client_balance.available << " < " << proposal.getTotalStorageFee();
      deal->message = ss.str();
      return false;
    }

    return true;
  }

  outcome::result<boost::optional<CID>>
  StorageProviderImpl::ensureProviderFunds(std::shared_ptr<MinerDeal> deal) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    auto proposal = deal->client_deal_proposal.proposal;
    OUTCOME_TRY(worker_info,
                api_->StateMinerInfo(proposal.provider, tipset_key));
    OUTCOME_TRY(maybe_cid,
                api_->MarketEnsureAvailable(proposal.provider,
                                            worker_info.worker,
                                            proposal.provider_collateral,
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
    CID cid = signed_message.getCid();
    OUTCOME_TRY(str_cid, cid.toString());
    logger_->debug("Deal published with CID = " + str_cid);
    return std::move(cid);
  }

  void StorageProviderImpl::sendSignedResponse(
      std::shared_ptr<MinerDeal> deal) {
    Response response{.state = deal->state,
                      .message = deal->message,
                      .proposal = deal->proposal_cid,
                      // if deal is not published, set any valid value
                      .publish_message = deal->publish_cid
                                             ? deal->publish_cid.get()
                                             : deal->proposal_cid};
    // TODO sign response
    SignedResponse signed_response{.response = response};
    // TODO handle if stream is absent
    auto stream = connections_[deal->proposal_cid];
    stream->write(
        signed_response,
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

  outcome::result<PieceInfo> StorageProviderImpl::locatePiece(
      std::shared_ptr<MinerDeal> deal) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(
        piece_info,
        miner_api_->LocatePieceForDealWithinSector(deal->deal_id, tipset_key));
    return piece_info;
  }

  outcome::result<void> StorageProviderImpl::recordPieceInfo(
      std::shared_ptr<MinerDeal> deal, const PieceInfo &piece_info) {
    // TODO handle metadata file

    std::map<CID, PayloadLocation> locations;
    locations[deal->ref.root] = {};
    OUTCOME_TRY(piece_storage_->addPayloadLocations(
        deal->client_deal_proposal.proposal.piece_cid, locations));
    OUTCOME_TRY(piece_storage_->addPieceInfo(
        deal->client_deal_proposal.proposal.piece_cid, piece_info));

    return outcome::success();
  }  // namespace fc::markets::storage::provider

  std::vector<ProviderTransition> StorageProviderImpl::makeFSMTransitions() {
    return {
        ProviderTransition(ProviderEvent::ProviderEventOpen)
            .from(StorageDealStatus::STORAGE_DEAL_UNKNOWN)
            .to(StorageDealStatus::STORAGE_DEAL_VALIDATING)
            .action(CALLBACK_ACTION(onProviderEventOpen)),
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
            .fromAny()
            .to(StorageDealStatus::STORAGE_DEAL_ERROR)
            .action(CALLBACK_ACTION(onProviderEventFailed))};
  }

  void StorageProviderImpl::onProviderEventOpen(std::shared_ptr<MinerDeal> deal,
                                                ProviderEvent event,
                                                StorageDealStatus from,
                                                StorageDealStatus to) {
    auto verified = verifyDealProposal(deal);
    if (verified.has_error()) {
      deal->message =
          "Deal proposal verify error: " + verified.error().message();
      OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventFailed));
      return;
    }
    if (!verified.value()) {
      OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventFailed));
      return;
    }
    OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventDealAccepted));
  }

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
    // add log
  }

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
      deal->message = "Publish deal error: " + maybe_cid.error().message();
      OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventFailed));
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
      StorageDealStatus to) {
    // todo no need in error states
  }

  void StorageProviderImpl::onProviderEventDataTransferInitiated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // todo log?
  }

  void StorageProviderImpl::onProviderEventDataTransferCompleted(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // todo verify data
  }

  void StorageProviderImpl::onProviderEventGeneratePieceCIDFailed(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // todo no need in error states
  }

  void StorageProviderImpl::onProviderEventVerifiedData(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto maybe_cid = ensureProviderFunds(deal);
    if (maybe_cid.has_error()) {
      deal->message =
          "Ensure provider funds failed: " + maybe_cid.error().message();
      OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventFailed));
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
      StorageDealStatus to) {
    // todo no need in error states
  }

  void StorageProviderImpl::onProviderEventDealPublishInitiated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto maybe_wait = api_->StateWaitMsg(deal->publish_cid.get());
    if (maybe_wait.has_error()) {
      deal->message =
          "Wait for publish failed: " + maybe_wait.error().message();
      OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventFailed));
      return;
    }
    maybe_wait.value().wait(
        [self{shared_from_this()}, deal, to](outcome::result<MsgWait> result) {
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
          deal->state = to;
          self->sendSignedResponse(deal);
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
      StorageDealStatus to) {
    // todo no need in error states
  }

  void StorageProviderImpl::onProviderEventFileStoreErrored(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // todo no need in error states
  }

  void StorageProviderImpl::onProviderEventDealHandoffFailed(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // todo no need in error states
  }

  void StorageProviderImpl::onProviderEventDealHandedOff(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO verify deal activated
    // on deal sector committed
    OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventDealActivated));
  }

  void StorageProviderImpl::onProviderEventDealActivationFailed(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // todo no need in error states
  }

  void StorageProviderImpl::onProviderEventUnableToLocatePiece(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // todo no need in error states
  }

  void StorageProviderImpl::onProviderEventDealActivated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto maybe_piece_info = locatePiece(deal);
    if (maybe_piece_info.has_error()) {
      OUTCOME_EXCEPT(
          fsm_->send(deal, ProviderEvent::ProviderEventUnableToLocatePiece));
    }
    if (recordPieceInfo(deal, maybe_piece_info.value()).has_error()) {
      OUTCOME_EXCEPT(
          fsm_->send(deal, ProviderEvent::ProviderEventPieceStoreErrored));
    }
    OUTCOME_EXCEPT(fsm_->send(deal, ProviderEvent::ProviderEventDealCompleted));
  }

  void StorageProviderImpl::onProviderEventPieceStoreErrored(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // todo no need in error states
  }

  void StorageProviderImpl::onProviderEventReadMetadataErrored(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // todo no need in error states
  }

  void StorageProviderImpl::onProviderEventDealCompleted(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    logger_->debug("Deal completed");
    // todo clean up
  }

  void StorageProviderImpl::onProviderEventFailed(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    logger_->debug("Deal failed with message: " + deal->message);
    deal->state = to;
    sendSignedResponse(deal);
  }

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
