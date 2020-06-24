/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "provider_impl.hpp"

#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "common/todo_error.hpp"
#include "data_transfer/impl/graphsync/graphsync_manager.hpp"
#include "host/context/host_context.hpp"
#include "host/context/impl/host_context_impl.hpp"
#include "markets/storage/common.hpp"
#include "markets/storage/provider/impl/provider_data_transfer_request_validator.hpp"
#include "markets/storage/provider/storage_provider_error.hpp"
#include "markets/storage/provider/stored_ask.hpp"
#include "markets/storage/storage_datatransfer_voucher.hpp"
#include "storage/ipfs/graphsync/impl/graphsync_impl.hpp"
#include "storage/piece/impl/piece_storage_impl.hpp"
#include "vm/actor/builtin/market/actor.hpp"

#define CALLBACK_ACTION(_action)                      \
  [this](auto deal, auto event, auto from, auto to) { \
    logger_->debug("Provider FSM " #_action);         \
    _action(deal, event, from, to);                   \
    deal->state = to;                                 \
  }

#define FSM_HALT_ON_ERROR(result, msg, deal)                            \
  if (result.has_error()) {                                             \
    deal->message = msg + std::string(". ") + result.error().message(); \
    FSM_SEND(deal, ProviderEvent::ProviderEventFailed);                 \
    return;                                                             \
  }

#define SELF_FSM_HALT_ON_ERROR(result, msg, deal)                       \
  if (result.has_error()) {                                             \
    deal->message = msg + std::string(". ") + result.error().message(); \
    SELF_FSM_SEND(deal, ProviderEvent::ProviderEventFailed);            \
    return;                                                             \
  }

namespace fc::markets::storage::provider {
  using api::MsgWait;
  using data_transfer::Voucher;
  using data_transfer::graphsync::GraphSyncManager;
  using fc::storage::ipfs::graphsync::GraphsyncImpl;
  using fc::storage::piece::PayloadLocation;
  using fc::storage::piece::PieceStorageImpl;
  using host::HostContext;
  using host::HostContextImpl;
  using vm::VMExitCode;
  using vm::actor::MethodParams;
  using vm::actor::builtin::market::PublishStorageDeals;
  using vm::message::kMessageVersion;
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
      std::shared_ptr<PieceIO> piece_io,
      std::shared_ptr<FileStore> filestore)
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
        piece_storage_{std::make_shared<PieceStorageImpl>(datastore)},
        filestore_{filestore} {
    auto scheduler = std::make_shared<libp2p::protocol::AsioScheduler>(
        *context_, libp2p::protocol::SchedulerConfig{});
    auto graphsync =
        std::make_shared<GraphsyncImpl>(host_, std::move(scheduler));
    datatransfer_ = std::make_shared<GraphSyncManager>(host_, graphsync);
  }

  outcome::result<void> StorageProviderImpl::init() {
    OUTCOME_TRY(filestore_->createDirectories(kFilestoreTempDir));

    // init fsm transitions
    std::shared_ptr<HostContext> fsm_context =
        std::make_shared<HostContextImpl>(context_);
    fsm_ = std::make_shared<ProviderFSM>(makeFSMTransitions(), fsm_context);

    // register request validator
    auto state_store = std::make_shared<ProviderFsmStateStore>(fsm_);
    auto validator =
        std::make_shared<ProviderDataTransferRequestValidator>(state_store);
    OUTCOME_TRY(datatransfer_->init(StorageDataTransferVoucherType, validator));

    return outcome::success();
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

  outcome::result<void> StorageProviderImpl::stop() {
    fsm_->stop();
    OUTCOME_TRY(network_->stopHandlingRequests());
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto &[_, stream] : connections_) {
      network_->closeStreamGracefully(stream);
    }
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

  outcome::result<MinerDeal> StorageProviderImpl::getDeal(
      const CID &proposal_cid) const {
    for (const auto &it : fsm_->list()) {
      if (it.first->proposal_cid == proposal_cid) {
        return *it.first;
      }
    }
    return StorageMarketProviderError::LOCAL_DEAL_NOT_FOUND;
  }

  outcome::result<void> StorageProviderImpl::addStorageCollateral(
      const TokenAmount &amount) {
    // TODO
    return TodoError::ERROR;
  }

  outcome::result<TokenAmount> StorageProviderImpl::getStorageCollateral() {
    // TODO
    return outcome::failure(TodoError::ERROR);
  }

  outcome::result<void> StorageProviderImpl::importDataForDeal(
      const CID &proposal_cid, const Buffer &data) {
    auto fsm_state_table = fsm_->list();
    auto found_fsm_entity =
        std::find_if(fsm_state_table.begin(),
                     fsm_state_table.end(),
                     [proposal_cid](const auto &it) -> bool {
                       if (it.first->proposal_cid == proposal_cid) return true;
                       return false;
                     });
    if (found_fsm_entity == fsm_state_table.end()) {
      return StorageMarketProviderError::LOCAL_DEAL_NOT_FOUND;
    }
    auto deal = found_fsm_entity->first;

    OUTCOME_TRY(piece_commitment,
                piece_io_->generatePieceCommitment(registered_proof_, data));

    if (piece_commitment.first
        != deal->client_deal_proposal.proposal.piece_cid) {
      return StorageMarketProviderError::PIECE_CID_DOESNT_MATCH;
    }

    OUTCOME_TRY(cid_str, proposal_cid.toString());
    Path path = kFilestoreTempDir + cid_str;
    OUTCOME_TRY(file, filestore_->create(path));
    deal->piece_path = path;
    OUTCOME_TRY(file->write(0, data));

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

    stream->read<Proposal>(
        [self{shared_from_this()}, stream](outcome::result<Proposal> proposal) {
          if (!self->hasValue(proposal, "Read proposal error: ", stream))
            return;
          auto proposal_cid{proposal.value().deal_proposal.cid()};
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
                        .proposal_cid = proposal_cid,
                        .add_funds_cid = boost::none,
                        .publish_cid = boost::none,
                        .client = remote_peer_info,
                        .state = StorageDealStatus::STORAGE_DEAL_UNKNOWN,
                        .piece_path = {},
                        .metadata_path = {},
                        .message = {},
                        .ref = proposal.value().piece,
                        .deal_id = {}});
          std::lock_guard<std::mutex> lock(self->connections_mutex_);
          self->connections_.emplace(proposal_cid, stream);
          OUTCOME_EXCEPT(
              self->fsm_->begin(deal, StorageDealStatus::STORAGE_DEAL_UNKNOWN));
          SELF_FSM_SEND(deal, ProviderEvent::ProviderEventOpen);
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
    TokenAmount available = client_balance.escrow - client_balance.locked;
    if (available < proposal.getTotalStorageFee()) {
      std::stringstream ss;
      ss << "Deal proposal verification failed, client market available "
            "balance too small: "
         << available << " < " << proposal.getTotalStorageFee();
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
    UnsignedMessage unsigned_message(kMessageVersion,
                                     vm::actor::kStorageMarketAddress,
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

  outcome::result<void> StorageProviderImpl::sendSignedResponse(
      std::shared_ptr<MinerDeal> deal) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(worker_info,
                api_->StateMinerInfo(
                    deal->client_deal_proposal.proposal.provider, tipset_key));
    OUTCOME_TRY(worker_key_address,
                api_->StateAccountKey(worker_info.worker, tipset_key));
    Response response{.state = deal->state,
                      .message = deal->message,
                      .proposal = deal->proposal_cid,
                      // if deal is not published, set any valid value
                      .publish_message = deal->publish_cid
                                             ? deal->publish_cid.get()
                                             : deal->proposal_cid};
    OUTCOME_TRY(encoded_response, codec::cbor::encode(response));
    OUTCOME_TRY(signature,
                api_->WalletSign(worker_key_address, encoded_response));
    SignedResponse signed_response{.response = response,
                                   .signature = signature};
    OUTCOME_TRY(stream, getStream(deal->proposal_cid));
    stream->write(signed_response,
                  [self{shared_from_this()}, stream, deal](
                      outcome::result<size_t> maybe_res) {
                    if (maybe_res.has_error()) {
                      // assume client disconnected
                      self->logger_->error("Write deal response error. "
                                           + maybe_res.error().message());
                      return;
                    }
                    self->network_->closeStreamGracefully(stream);
                    SELF_FSM_SEND(deal,
                                  ProviderEvent::ProviderEventDealPublished);
                  });

    return outcome::success();
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
    std::map<CID, PayloadLocation> locations;
    if (!deal->metadata_path.empty()) {
      // TODO (a.chernyshov) load block locations from metadata file
      // https://github.com/filecoin-project/go-fil-markets/blob/master/storagemarket/impl/providerstates/provider_states.go#L310
    } else {
      locations[deal->ref.root] = {};
    }
    OUTCOME_TRY(piece_storage_->addPayloadLocations(
        deal->client_deal_proposal.proposal.piece_cid, locations));
    OUTCOME_TRY(piece_storage_->addPieceInfo(
        deal->client_deal_proposal.proposal.piece_cid, piece_info));

    return outcome::success();
  }

  outcome::result<std::shared_ptr<CborStream>> StorageProviderImpl::getStream(
      const CID &proposal_cid) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto stream_it = connections_.find(proposal_cid);
    if (stream_it == connections_.end()) {
      return StorageProviderError::STREAM_LOOKUP_ERROR;
    }
    return stream_it->second;
  }

  outcome::result<void> StorageProviderImpl::finalizeDeal(
      std::shared_ptr<MinerDeal> deal) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto stream_it = connections_.find(deal->proposal_cid);
    if (stream_it != connections_.end()) {
      network_->closeStreamGracefully(stream_it->second);
    }
    connections_.erase(stream_it);
    if (!deal->piece_path.empty()) {
      OUTCOME_TRY(filestore_->remove(deal->piece_path));
    }
    if (!deal->metadata_path.empty()) {
      OUTCOME_TRY(filestore_->remove(deal->metadata_path));
    }
    return outcome::success();
  }

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
        ProviderTransition(ProviderEvent::ProviderEventDataTransferInitiated)
            .from(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
            .to(StorageDealStatus::STORAGE_DEAL_TRANSFERRING)
            .action(CALLBACK_ACTION(onProviderEventDataTransferInitiated)),
        ProviderTransition(ProviderEvent::ProviderEventDataTransferCompleted)
            .from(StorageDealStatus::STORAGE_DEAL_TRANSFERRING)
            .to(StorageDealStatus::STORAGE_DEAL_VERIFY_DATA)
            .action(CALLBACK_ACTION(onProviderEventDataTransferCompleted)),
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
        ProviderTransition(ProviderEvent::ProviderEventDealPublished)
            .from(StorageDealStatus::STORAGE_DEAL_PUBLISHING)
            .to(StorageDealStatus::STORAGE_DEAL_STAGED)
            .action(CALLBACK_ACTION(onProviderEventDealPublished)),
        ProviderTransition(ProviderEvent::ProviderEventDealHandedOff)
            .from(StorageDealStatus::STORAGE_DEAL_STAGED)
            .to(StorageDealStatus::STORAGE_DEAL_SEALING)
            .action(CALLBACK_ACTION(onProviderEventDealHandedOff)),
        ProviderTransition(ProviderEvent::ProviderEventDealActivated)
            .from(StorageDealStatus::STORAGE_DEAL_SEALING)
            .to(StorageDealStatus::STORAGE_DEAL_ACTIVE)
            .action(CALLBACK_ACTION(onProviderEventDealActivated)),
        ProviderTransition(ProviderEvent::ProviderEventDealCompleted)
            .from(StorageDealStatus::STORAGE_DEAL_ACTIVE)
            .to(StorageDealStatus::STORAGE_DEAL_COMPLETED)
            .action(CALLBACK_ACTION(onProviderEventDealCompleted)),
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
    FSM_HALT_ON_ERROR(verified, "Deal proposal verify error", deal);
    if (!verified.value()) {
      FSM_SEND(deal, ProviderEvent::ProviderEventFailed);
      return;
    }
    FSM_SEND(deal, ProviderEvent::ProviderEventDealAccepted);
  }

  void StorageProviderImpl::onProviderEventDealAccepted(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    if (deal->ref.transfer_type == kTransferTypeManual) {
      FSM_SEND(deal, ProviderEvent::ProviderEventWaitingForManualData);
      return;
    }

    auto voucher_encoded = codec::cbor::encode(
        StorageDataTransferVoucher{.proposal_cid = deal->proposal_cid});
    FSM_HALT_ON_ERROR(
        voucher_encoded, "Encode data transfer voucher error", deal);
    Voucher voucher{.type = StorageDataTransferVoucherType,
                    .bytes = voucher_encoded.value().toVector()};
    // TODO (a.chernyshov) implement selectors and deserialize from
    // request.selector
    auto selector = std::make_shared<Selector>();
    FSM_HALT_ON_ERROR(datatransfer_->openPullDataChannel(
                          deal->client, voucher, deal->ref.root, selector),
                      "Data transfer open pull data channel error",
                      deal);
    FSM_SEND(deal, ProviderEvent::ProviderEventDataTransferInitiated);
  }

  void StorageProviderImpl::onProviderEventWaitingForManualData(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    logger_->debug("Waiting for importDataForDeal() call");
  }

  void StorageProviderImpl::onProviderEventFundingInitiated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto maybe_wait = api_->StateWaitMsg(deal->add_funds_cid.get());
    FSM_HALT_ON_ERROR(maybe_wait, "Wait for funding error", deal);
    maybe_wait.value().wait(
        [self{shared_from_this()}, deal](outcome::result<MsgWait> result) {
          SELF_FSM_HALT_ON_ERROR(result, "Wait for funding error", deal);
          if (result.value().receipt.exit_code != VMExitCode::Ok) {
            deal->message = "Funding exit code "
                            + std::to_string(static_cast<uint64_t>(
                                result.value().receipt.exit_code));
            SELF_FSM_SEND(deal, ProviderEvent::ProviderEventFailed);
            return;
          }
          SELF_FSM_SEND(deal, ProviderEvent::ProviderEventFunded);
        });
  }

  void StorageProviderImpl::onProviderEventFunded(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto maybe_cid = publishDeal(deal);
    FSM_HALT_ON_ERROR(maybe_cid, "Publish deal error", deal);
    deal->publish_cid = maybe_cid.value();
    FSM_SEND(deal, ProviderEvent::ProviderEventDealPublishInitiated);
  }

  void StorageProviderImpl::onProviderEventDataTransferInitiated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // nothing, wait for data transfer completed
  }

  void StorageProviderImpl::onProviderEventDataTransferCompleted(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // todo verify data
    // pieceCid, piecePath, metadataPath = generatePieceCommitmentToFile
    //  - if universalRetrievalEnabled GeneratePieceCommitmentWithMetadata
    //    - generates a piece commitment along with block metadata
    //  - else pio.GeneratePieceCommitmentToFile
    // if compare pieceCid != deal.Proposal.PieceCID error
    // else ok
  }

  void StorageProviderImpl::onProviderEventVerifiedData(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto funding_cid = ensureProviderFunds(deal);
    FSM_HALT_ON_ERROR(funding_cid, "Ensure provider funds failed", deal);

    // funding message was sent
    if (funding_cid.value().has_value()) {
      deal->add_funds_cid = *funding_cid.value();
      FSM_SEND(deal, ProviderEvent::ProviderEventFundingInitiated);
      return;
    }

    FSM_SEND(deal, ProviderEvent::ProviderEventFunded);
  }

  void StorageProviderImpl::onProviderEventDealPublishInitiated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto maybe_wait = api_->StateWaitMsg(deal->publish_cid.get());
    FSM_HALT_ON_ERROR(maybe_wait, "Wait for publish failed", deal);
    maybe_wait.value().wait([self{shared_from_this()}, deal, to](
                                outcome::result<MsgWait> result) {
      SELF_FSM_HALT_ON_ERROR(
          result, "Publish storage deal message error", deal);
      if (result.value().receipt.exit_code != VMExitCode::Ok) {
        deal->message = "Publish storage deal exit code "
                        + std::to_string(static_cast<uint64_t>(
                            result.value().receipt.exit_code));
        SELF_FSM_SEND(deal, ProviderEvent::ProviderEventFailed);
        return;
      }
      auto maybe_res = codec::cbor::decode<PublishStorageDeals::Result>(
          result.value().receipt.return_value);
      SELF_FSM_HALT_ON_ERROR(
          maybe_res, "Publish storage deal decode result error", deal);
      if (maybe_res.value().deals.size() != 1) {
        deal->message = "Publish storage deal result size error";
        SELF_FSM_SEND(deal, ProviderEvent::ProviderEventFailed);
        return;
      }
      deal->deal_id = maybe_res.value().deals.front();
      deal->state = to;
      SELF_FSM_HALT_ON_ERROR(
          self->sendSignedResponse(deal), "Error when sending response", deal);
    });
  }

  void StorageProviderImpl::onProviderEventDealPublished(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO hand off
    // miner_node_api.addPiece
    FSM_SEND(deal, ProviderEvent::ProviderEventDealHandedOff);
  }

  void StorageProviderImpl::onProviderEventDealHandedOff(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO verify deal activated
    // on deal sector committed
    FSM_SEND(deal, ProviderEvent::ProviderEventDealActivated);
  }

  void StorageProviderImpl::onProviderEventDealActivated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto maybe_piece_info = locatePiece(deal);
    FSM_HALT_ON_ERROR(maybe_piece_info, "Unable to locate piece", deal);
    FSM_HALT_ON_ERROR(recordPieceInfo(deal, maybe_piece_info.value()),
                      "Record piece failed",
                      deal);
    FSM_SEND(deal, ProviderEvent::ProviderEventDealCompleted);
  }

  void StorageProviderImpl::onProviderEventDealCompleted(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    logger_->debug("Deal completed");
    auto res = finalizeDeal(deal);
    if (res.has_error()) {
      logger_->error("Deal finalization error. " + res.error().message());
    }
  }

  void StorageProviderImpl::onProviderEventFailed(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    logger_->error("Deal failed with message: " + deal->message);
    deal->state = to;
    auto response_res = sendSignedResponse(deal);
    if (response_res.has_error()) {
      logger_->error("Error when sending error response. "
                     + response_res.error().message());
    }
    auto res = finalizeDeal(deal);
    if (res.has_error()) {
      logger_->error("Deal finalization error. " + res.error().message());
    }
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
