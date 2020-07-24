/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage_market_client_impl.hpp"

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include "codec/cbor/cbor.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "data_transfer/impl/graphsync/graphsync_manager.hpp"
#include "host/context/impl/host_context_impl.hpp"
#include "markets/common.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/client/impl/client_data_transfer_request_validator.hpp"
#include "markets/storage/storage_datatransfer_voucher.hpp"
#include "storage/ipfs/graphsync/impl/graphsync_impl.hpp"
#include "vm/message/message.hpp"
#include "vm/message/message_util.hpp"

#define CALLBACK_ACTION(_action)                      \
  [this](auto deal, auto event, auto from, auto to) { \
    logger_->debug("Client FSM " #_action);           \
    _action(deal, event, from, to);                   \
    deal->state = to;                                 \
  }

#define FSM_HALT_ON_ERROR(result, msg, deal)                            \
  if (result.has_error()) {                                             \
    deal->message = msg + std::string(". ") + result.error().message(); \
    FSM_SEND(deal, ClientEvent::ClientEventFailed);                     \
    return;                                                             \
  }

#define SELF_FSM_HALT_ON_ERROR(result, msg, deal)                       \
  if (result.has_error()) {                                             \
    deal->message = msg + std::string(". ") + result.error().message(); \
    SELF_FSM_SEND(deal, ClientEvent::ClientEventFailed);                \
    return;                                                             \
  }

namespace fc::markets::storage::client {
  using api::MsgWait;
  using data_transfer::graphsync::GraphSyncManager;
  using fc::storage::ipfs::graphsync::GraphsyncImpl;
  using host::HostContext;
  using host::HostContextImpl;
  using libp2p::peer::PeerId;
  using primitives::BigInt;
  using primitives::GasAmount;
  using vm::VMExitCode;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::builtin::market::PublishStorageDeals;
  using vm::message::kDefaultGasLimit;
  using vm::message::kDefaultGasPrice;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  StorageMarketClientImpl::StorageMarketClientImpl(
      std::shared_ptr<Host> host,
      std::shared_ptr<boost::asio::io_context> context,
      std::shared_ptr<Datastore> datastore,
      std::shared_ptr<Api> api,
      std::shared_ptr<PieceIO> piece_io)
      : host_{std::make_shared<CborHost>(host)},
        context_{std::move(context)},
        api_{std::move(api)},
        piece_io_{std::move(piece_io)},
        discovery_{std::make_shared<Discovery>(datastore)} {
    auto scheduler = std::make_shared<libp2p::protocol::AsioScheduler>(
        *context_, libp2p::protocol::SchedulerConfig{});
    auto graphsync =
        std::make_shared<GraphsyncImpl>(host, std::move(scheduler));
    datatransfer_ = std::make_shared<GraphSyncManager>(host, graphsync);
  }

  outcome::result<void> StorageMarketClientImpl::init() {
    // init fsm transitions
    std::shared_ptr<HostContext> fsm_context =
        std::make_shared<HostContextImpl>(context_);
    fsm_ = std::make_shared<ClientFSM>(makeFSMTransitions(), fsm_context);

    // register request validator
    auto state_store = std::make_shared<ClientFsmStateStore>(fsm_);
    auto validator =
        std::make_shared<ClientDataTransferRequestValidator>(state_store);
    OUTCOME_TRY(datatransfer_->init(StorageDataTransferVoucherType, validator));

    return outcome::success();
  }

  void StorageMarketClientImpl::run() {}

  outcome::result<void> StorageMarketClientImpl::stop() {
    fsm_->stop();
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto &[_, stream] : connections_) {
      closeStreamGracefully(stream, logger_);
    }
    return outcome::success();
  }

  outcome::result<std::vector<StorageProviderInfo>>
  StorageMarketClientImpl::listProviders() const {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(miners, api_->StateListMiners(tipset_key));
    std::vector<StorageProviderInfo> storage_providers;
    for (const auto &miner_address : miners) {
      OUTCOME_TRY(miner_info, api_->StateMinerInfo(miner_address, tipset_key));
      PeerInfo peer_info{.id = miner_info.peer_id, .addresses = {}};
      storage_providers.push_back(
          StorageProviderInfo{.address = miner_address,
                              .owner = {},
                              .worker = miner_info.worker,
                              .sector_size = miner_info.sector_size,
                              .peer_info = peer_info});
    }
    return storage_providers;
  }

  outcome::result<std::vector<StorageDeal>> StorageMarketClientImpl::listDeals(
      const Address &address) const {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(all_deals, api_->StateMarketDeals(tipset_key));
    std::vector<StorageDeal> client_deals;
    for (const auto &deal : all_deals) {
      if (deal.second.proposal.client == address) {
        client_deals.emplace_back(deal.second);
      }
    }
    return client_deals;
  }

  outcome::result<std::vector<ClientDeal>>
  StorageMarketClientImpl::listLocalDeals() const {
    std::vector<ClientDeal> res;
    for (const auto &it : fsm_->list()) {
      res.push_back(*it.first);
    }
    return res;
  }

  outcome::result<ClientDeal> StorageMarketClientImpl::getLocalDeal(
      const CID &proposal_cid) const {
    for (const auto &it : fsm_->list()) {
      if (it.first->proposal_cid == proposal_cid) {
        return *it.first;
      }
    }
    return StorageMarketClientError::kLocalDealNotFound;
  }

  void StorageMarketClientImpl::getAsk(
      const StorageProviderInfo &info,
      const SignedAskHandler &signed_ask_handler) {
    host_->newCborStream(
        info.peer_info,
        kAskProtocolId,
        [self{shared_from_this()}, info, signed_ask_handler](
            auto &&stream_res) {
          if (stream_res.has_error()) {
            self->logger_->error("Cannot open stream to "
                                 + peerInfoToPrettyString(info.peer_info)
                                 + stream_res.error().message());
            signed_ask_handler(outcome::failure(stream_res.error()));
            return;
          }
          auto stream = std::move(stream_res.value());
          AskRequest request{.miner = info.address};
          stream->write(request,
                        [self, info, stream, signed_ask_handler](
                            outcome::result<size_t> written) {
                          if (!self->hasValue(written,
                                              "Cannot send request",
                                              stream,
                                              signed_ask_handler)) {
                            return;
                          }
                          stream->template read<AskResponse>(
                              [self, info, stream, signed_ask_handler](
                                  outcome::result<AskResponse> response) {
                                auto validated_ask_response =
                                    self->validateAskResponse(response, info);
                                signed_ask_handler(validated_ask_response);
                                closeStreamGracefully(stream, self->logger_);
                              });
                        });
        });
  }

  outcome::result<CID> StorageMarketClientImpl::proposeStorageDeal(
      const Address &client_address,
      const StorageProviderInfo &provider_info,
      const DataRef &data_ref,
      const ChainEpoch &start_epoch,
      const ChainEpoch &end_epoch,
      const TokenAmount &price,
      const TokenAmount &collateral,
      const RegisteredProof &registered_proof) {
    OUTCOME_TRY(comm_p_res, calculateCommP(registered_proof, data_ref));
    CID comm_p = comm_p_res.first;
    UnpaddedPieceSize piece_size = comm_p_res.second;
    if (piece_size.padded() > provider_info.sector_size) {
      return StorageMarketClientError::kPieceSizeGreaterSectorSize;
    }

    DealProposal deal_proposal{
        .piece_cid = comm_p,
        .piece_size = piece_size.padded(),
        .verified = false,
        .client = client_address,
        .provider = provider_info.address,
        .start_epoch = start_epoch,
        .end_epoch = end_epoch,
        .storage_price_per_epoch = price,
        .provider_collateral = static_cast<uint64_t>(piece_size),
        .client_collateral = 0};
    OUTCOME_TRY(signed_proposal, signProposal(client_address, deal_proposal));
    auto proposal_cid{signed_proposal.cid()};

    auto client_deal = std::make_shared<ClientDeal>(
        ClientDeal{.client_deal_proposal = signed_proposal,
                   .proposal_cid = proposal_cid,
                   .add_funds_cid = boost::none,
                   .state = StorageDealStatus::STORAGE_DEAL_UNKNOWN,
                   .miner = provider_info.peer_info,
                   .miner_worker = provider_info.worker,
                   .deal_id = {},
                   .data_ref = data_ref,
                   .message = {},
                   .publish_message = {}});
    OUTCOME_TRY(
        fsm_->begin(client_deal, StorageDealStatus::STORAGE_DEAL_UNKNOWN));

    host_->newCborStream(
        provider_info.peer_info,
        kDealProtocolId,
        [self{shared_from_this()}, provider_info, client_deal, proposal_cid](
            outcome::result<std::shared_ptr<CborStream>> stream) {
          SELF_FSM_HALT_ON_ERROR(
              stream,
              "Cannot open stream to "
                  + peerInfoToPrettyString(provider_info.peer_info),
              client_deal);
          self->logger_->debug(
              "DealStream opened to "
              + peerInfoToPrettyString(provider_info.peer_info));

          std::lock_guard<std::mutex> lock(self->connections_mutex_);
          self->connections_.emplace(proposal_cid, stream.value());
          SELF_FSM_SEND(client_deal, ClientEvent::ClientEventOpen);
        });

    OUTCOME_TRY(discovery_->addPeer(data_ref.root, provider_info.peer_info));

    return client_deal->proposal_cid;
  }

  outcome::result<StorageParticipantBalance>
  StorageMarketClientImpl::getPaymentEscrow(const Address &address) const {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(balance, api_->StateMarketBalance(address, tipset_key));
    return StorageParticipantBalance{balance.locked,
                                     balance.escrow - balance.locked};
  }

  outcome::result<void> StorageMarketClientImpl::addPaymentEscrow(
      const Address &address, const TokenAmount &amount) {
    UnsignedMessage unsigned_message{
        kStorageMarketAddress,
        address,
        {},
        amount,
        kDefaultGasPrice,
        kDefaultGasLimit,
        vm::actor::builtin::market::AddBalance::Number,
        {}};
    OUTCOME_TRY(signed_message, api_->MpoolPushMessage(unsigned_message));
    OUTCOME_TRY(message_cid, vm::message::cid(signed_message));
    OUTCOME_TRY(msg_wait, api_->StateWaitMsg(message_cid));
    OUTCOME_TRY(msg_state, msg_wait.waitSync());
    if (msg_state.receipt.exit_code != VMExitCode::kOk) {
      return StorageMarketClientError::kAddFundsCallError;
    }
    return outcome::success();
  }

  outcome::result<SignedStorageAsk>
  StorageMarketClientImpl::validateAskResponse(
      const outcome::result<AskResponse> &response,
      const StorageProviderInfo &info) const {
    if (response.has_error()) {
      return response.error();
    }
    if (response.value().ask.ask.miner != info.address) {
      return StorageMarketClientError::kWrongMiner;
    }
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(miner_info, api_->StateMinerInfo(info.address, tipset_key));
    OUTCOME_TRY(ask_bytes, codec::cbor::encode(response.value().ask.ask));
    OUTCOME_TRY(
        signature_valid,
        api_->WalletVerify(
            miner_info.worker, ask_bytes, response.value().ask.signature));
    if (!signature_valid) {
      logger_->debug("Ask response signature invalid");
      return StorageMarketClientError::kSignatureInvalid;
    }
    return response.value().ask;
  }

  outcome::result<std::pair<CID, UnpaddedPieceSize>>
  StorageMarketClientImpl::calculateCommP(
      const RegisteredProof &registered_proof, const DataRef &data_ref) const {
    if (data_ref.piece_cid.has_value()) {
      return std::pair(data_ref.piece_cid.value(), data_ref.piece_size);
    }
    if (data_ref.transfer_type == kTransferTypeManual) {
      return StorageMarketClientError::kPieceDataNotSetManualTransfer;
    }

    // TODO (a.chernyshov) selector builder
    // https://github.com/filecoin-project/go-fil-markets/blob/master/storagemarket/impl/clientutils/clientutils.go#L31
    return piece_io_->generatePieceCommitment(
        registered_proof, data_ref.root, {});
  }

  outcome::result<ClientDealProposal> StorageMarketClientImpl::signProposal(
      const Address &address, const DealProposal &proposal) const {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(key_address, api_->StateAccountKey(address, tipset_key));
    OUTCOME_TRY(proposal_bytes, codec::cbor::encode(proposal));
    OUTCOME_TRY(signature, api_->WalletSign(key_address, proposal_bytes));
    return ClientDealProposal{.proposal = proposal,
                              .client_signature = signature};
  }

  outcome::result<boost::optional<CID>> StorageMarketClientImpl::ensureFunds(
      std::shared_ptr<ClientDeal> deal) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(
        maybe_cid,
        api_->MarketEnsureAvailable(
            deal->client_deal_proposal.proposal.client,
            deal->client_deal_proposal.proposal.client,
            deal->client_deal_proposal.proposal.clientBalanceRequirement(),
            tipset_key));
    return std::move(maybe_cid);
  }

  outcome::result<void> StorageMarketClientImpl::verifyDealResponseSignature(
      const SignedResponse &response, const std::shared_ptr<ClientDeal> &deal) {
    OUTCOME_TRY(response_bytes, codec::cbor::encode(response.response));
    OUTCOME_TRY(signature_valid,
                api_->WalletVerify(
                    deal->miner_worker, response_bytes, response.signature));
    if (!signature_valid) {
      return StorageMarketClientError::kSignatureInvalid;
    }
    return outcome::success();
  }

  outcome::result<bool> StorageMarketClientImpl::verifyDealPublished(
      std::shared_ptr<ClientDeal> deal) {
    OUTCOME_TRY(msg_wait, api_->StateWaitMsg(deal->publish_message));
    OUTCOME_TRY(msg_state, msg_wait.waitSync());
    if (msg_state.receipt.exit_code != VMExitCode::kOk) {
      deal->message =
          "Publish deal exit code "
          + std::to_string(static_cast<uint64_t>(msg_state.receipt.exit_code));
      return false;
    }

    // check if published
    OUTCOME_TRY(publish_message, api_->ChainGetMessage(deal->publish_message));
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(miner_info,
                api_->StateMinerInfo(
                    deal->client_deal_proposal.proposal.provider, tipset_key));
    OUTCOME_TRY(from_id_address,
                api_->StateLookupID(publish_message.from, tipset_key));
    if (from_id_address != miner_info.worker) {
      deal->message = "Publisher is not storage provider";
      return false;
    }
    if (publish_message.to != kStorageMarketAddress) {
      deal->message = "Receiver is not storage market actor";
      return false;
    }
    if (publish_message.method != PublishStorageDeals::Number) {
      deal->message = "Wrong method called";
      return false;
    }

    // check publish contains proposal cid
    OUTCOME_TRY(proposals,
                codec::cbor::decode<std::vector<ClientDealProposal>>(
                    publish_message.params));
    auto it = std::find(
        proposals.begin(), proposals.end(), deal->client_deal_proposal);
    if (it == proposals.end()) {
      OUTCOME_TRY(proposal_cid_str, deal->proposal_cid.toString());
      deal->message = "deal publish didn't contain our deal (message cid: "
                      + proposal_cid_str + ")";
    }

    // get proposal id from publish call return
    int index = std::distance(proposals.begin(), it);
    OUTCOME_TRY(publish_result,
                codec::cbor::decode<PublishStorageDeals::Result>(
                    msg_state.receipt.return_value));
    deal->deal_id = publish_result.deals[index];
    return true;
  }

  outcome::result<std::shared_ptr<CborStream>>
  StorageMarketClientImpl::getStream(const CID &proposal_cid) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto stream_it = connections_.find(proposal_cid);
    if (stream_it == connections_.end()) {
      return StorageMarketClientError::kStreamLookupError;
    }
    return stream_it->second;
  }

  void StorageMarketClientImpl::finalizeDeal(std::shared_ptr<ClientDeal> deal) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto stream_it = connections_.find(deal->proposal_cid);
    if (stream_it != connections_.end()) {
      closeStreamGracefully(stream_it->second, logger_);
      connections_.erase(stream_it);
    }
  }

  std::vector<ClientTransition> StorageMarketClientImpl::makeFSMTransitions() {
    return {ClientTransition(ClientEvent::ClientEventOpen)
                .from(StorageDealStatus::STORAGE_DEAL_UNKNOWN)
                .to(StorageDealStatus::STORAGE_DEAL_ENSURE_CLIENT_FUNDS)
                .action(CALLBACK_ACTION(onClientEventOpen)),
            ClientTransition(ClientEvent::ClientEventFundingInitiated)
                .from(StorageDealStatus::STORAGE_DEAL_ENSURE_CLIENT_FUNDS)
                .to(StorageDealStatus::STORAGE_DEAL_CLIENT_FUNDING)
                .action(CALLBACK_ACTION(onClientEventFundingInitiated)),
            ClientTransition(ClientEvent::ClientEventFundsEnsured)
                .fromMany(StorageDealStatus::STORAGE_DEAL_ENSURE_CLIENT_FUNDS,
                          StorageDealStatus::STORAGE_DEAL_CLIENT_FUNDING)
                .to(StorageDealStatus::STORAGE_DEAL_FUNDS_ENSURED)
                .action(CALLBACK_ACTION(onClientEventFundsEnsured)),
            ClientTransition(ClientEvent::ClientEventDealProposed)
                .from(StorageDealStatus::STORAGE_DEAL_FUNDS_ENSURED)
                .to(StorageDealStatus::STORAGE_DEAL_VALIDATING)
                .action(CALLBACK_ACTION(onClientEventDealProposed)),
            ClientTransition(ClientEvent::ClientEventDealRejected)
                .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
                .to(StorageDealStatus::STORAGE_DEAL_FAILING)
                .action(CALLBACK_ACTION(onClientEventDealRejected)),
            ClientTransition(ClientEvent::ClientEventDealAccepted)
                .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
                .to(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
                .action(CALLBACK_ACTION(onClientEventDealAccepted)),
            ClientTransition(ClientEvent::ClientEventDealPublished)
                .from(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
                .to(StorageDealStatus::STORAGE_DEAL_SEALING)
                .action(CALLBACK_ACTION(onClientEventDealPublished)),
            ClientTransition(ClientEvent::ClientEventDealActivated)
                .from(StorageDealStatus::STORAGE_DEAL_SEALING)
                .to(StorageDealStatus::STORAGE_DEAL_ACTIVE)
                .action(CALLBACK_ACTION(onClientEventDealActivated)),
            ClientTransition(ClientEvent::ClientEventFailed)
                .fromAny()
                .to(StorageDealStatus::STORAGE_DEAL_ERROR)
                .action(CALLBACK_ACTION(onClientEventFailed))};
  }

  void StorageMarketClientImpl::onClientEventOpen(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto maybe_cid = ensureFunds(deal);
    if (maybe_cid.has_error()) {
      deal->message = "Ensure funds failed: " + maybe_cid.error().message();
      FSM_SEND(deal, ClientEvent::ClientEventFailed);
      return;
    }

    // funding message was sent
    if (maybe_cid.value().has_value()) {
      deal->add_funds_cid = *maybe_cid.value();
      FSM_SEND(deal, ClientEvent::ClientEventFundingInitiated);
      return;
    }

    FSM_SEND(deal, ClientEvent::ClientEventFundsEnsured);
  }

  void StorageMarketClientImpl::onClientEventFundingInitiated(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto maybe_wait = api_->StateWaitMsg(deal->add_funds_cid.get());
    FSM_HALT_ON_ERROR(maybe_wait, "Wait for funding error", deal);
    maybe_wait.value().wait(
        [self{shared_from_this()}, deal](outcome::result<MsgWait> result) {
          SELF_FSM_HALT_ON_ERROR(result, "Wait for funding error", deal);
          if (result.value().receipt.exit_code != VMExitCode::kOk) {
            deal->message = "Funding exit code "
                            + std::to_string(static_cast<uint64_t>(
                                result.value().receipt.exit_code));
            SELF_FSM_SEND(deal, ClientEvent::ClientEventFailed);
            return;
          }
          SELF_FSM_SEND(deal, ClientEvent::ClientEventFundsEnsured);
        });
  }

  void StorageMarketClientImpl::onClientEventFundsEnsured(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto stream = getStream(deal->proposal_cid);
    FSM_HALT_ON_ERROR(stream, "Stream not found.", deal);

    Proposal proposal{.deal_proposal = deal->client_deal_proposal,
                      .piece = deal->data_ref};
    stream.value()->write(
        proposal,
        [self{shared_from_this()}, deal, stream](
            outcome::result<size_t> written) {
          SELF_FSM_HALT_ON_ERROR(written, "Send proposal error", deal);
          SELF_FSM_SEND(deal, ClientEvent::ClientEventDealProposed);
        });
  }

  void StorageMarketClientImpl::onClientEventDealProposed(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto stream_res = getStream(deal->proposal_cid);
    FSM_HALT_ON_ERROR(stream_res, "Stream not found.", deal);
    auto stream = std::move(stream_res.value());
    stream->read<SignedResponse>([self{shared_from_this()}, deal, stream](
                                     outcome::result<SignedResponse> response) {
      SELF_FSM_HALT_ON_ERROR(response, "Read response error", deal);
      SELF_FSM_HALT_ON_ERROR(
          self->verifyDealResponseSignature(response.value(), deal),
          "Response signature verification error",
          deal);
      if (response.value().response.proposal != deal->proposal_cid) {
        deal->message = "Response proposal cid doesn't match";
        SELF_FSM_SEND(deal, ClientEvent::ClientEventFailed);
        return;
      }
      if (response.value().response.state
          != StorageDealStatus::STORAGE_DEAL_PUBLISHING) {
        deal->message = response.value().response.message;
        SELF_FSM_SEND(deal, ClientEvent::ClientEventDealRejected);
        return;
      }
      deal->publish_message = response.value().response.publish_message;
      closeStreamGracefully(stream, self->logger_);
      SELF_FSM_SEND(deal, ClientEvent::ClientEventDealAccepted);
    });
  }

  void StorageMarketClientImpl::onClientEventDealRejected(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    logger_->debug("Deal rejected");
    FSM_SEND(deal, ClientEvent::ClientEventFailed);
  }

  void StorageMarketClientImpl::onClientEventDealAccepted(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto verified = verifyDealPublished(deal);
    FSM_HALT_ON_ERROR(verified, "Cannot get publish message", deal);
    if (!verified.value()) {
      FSM_SEND(deal, ClientEvent::ClientEventFailed);
      return;
    }
    FSM_SEND(deal, ClientEvent::ClientEventDealPublished);
  }

  void StorageMarketClientImpl::onClientEventDealPublished(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO (a.chernyshov) verify deal activated - on deal sector commit
    OUTCOME_EXCEPT(fsm_->send(deal, ClientEvent::ClientEventDealActivated));
  }

  void StorageMarketClientImpl::onClientEventDealActivated(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // final success state
    finalizeDeal(deal);
  }

  void StorageMarketClientImpl::onClientEventFailed(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // final error state
    std::stringstream ss;
    ss << "Proposal ";
    auto maybe_prpoposal_cid = deal->proposal_cid.toString();
    if (maybe_prpoposal_cid) ss << maybe_prpoposal_cid.value() << " ";
    ss << "failed. " << deal->message;
    logger_->error(ss.str());
    finalizeDeal(deal);
  }

}  // namespace fc::markets::storage::client

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::client,
                            StorageMarketClientError,
                            e) {
  using fc::markets::storage::client::StorageMarketClientError;

  switch (e) {
    case StorageMarketClientError::kWrongMiner:
      return "StorageMarketClientError: wrong miner address";
    case StorageMarketClientError::kSignatureInvalid:
      return "StorageMarketClientError: signature invalid";
    case StorageMarketClientError::kPieceDataNotSetManualTransfer:
      return "StorageMarketClientError: piece data is not set for manual "
             "transfer";
    case StorageMarketClientError::kPieceSizeGreaterSectorSize:
      return "StorageMarketClientError: piece size is greater sector size";
    case StorageMarketClientError::kAddFundsCallError:
      return "StorageMarketClientError: add funds method call returned error";
    case StorageMarketClientError::kLocalDealNotFound:
      return "StorageMarketClientError: local deal not found";
    case StorageMarketClientError::kStreamLookupError:
      return "StorageMarketClientError: stream look up error";
  }

  return "StorageMarketClientError: unknown error";
}
