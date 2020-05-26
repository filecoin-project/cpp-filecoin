/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage_market_client_impl.hpp"

#include <libp2p/peer/peer_id.hpp>
#include "codec/cbor/cbor.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "host/context/impl/host_context_impl.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "vm/message/message.hpp"
#include "vm/message/message_util.hpp"

#define CALLBACK_ACTION(_action)                                          \
  [self{shared_from_this()}](auto deal, auto event, auto from, auto to) { \
    self->logger_->debug("Client FSM " #_action);                         \
    self->_action(deal, event, from, to);                                 \
    deal->state = to;                                                     \
  }

#define FSM_SEND(client_deal, event) \
  OUTCOME_EXCEPT(fsm_->send(client_deal, event))

#define SELF_FSM_SEND(client_deal, event) \
  OUTCOME_EXCEPT(self->fsm_->send(client_deal, event))

#define SELF_FSM_HALT_ON_ERROR(result, msg, deal)                       \
  if (result.has_error()) {                                             \
    deal->message = msg + std::string(". ") + result.error().message(); \
    SELF_FSM_SEND(deal, ClientEvent::ClientEventFailed);                \
    return;                                                             \
  }

namespace fc::markets::storage::client {

  using host::HostContext;
  using host::HostContextImpl;
  using libp2p::peer::PeerId;
  using primitives::BigInt;
  using primitives::GasAmount;
  using vm::VMExitCode;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::builtin::market::getProposalCid;
  using vm::message::kMessageVersion;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  // from lotus
  // https://github.com/filecoin-project/lotus/blob/7e0be91cfd44c1664ac18f81080544b1341872f1/markets/storageadapter/client.go#L122
  const BigInt kGasPrice{0};
  const GasAmount kGasLimit{1000000};

  StorageMarketClientImpl::StorageMarketClientImpl(
      std::shared_ptr<Host> host,
      std::shared_ptr<boost::asio::io_context> context,
      std::shared_ptr<Datastore> datastore,
      std::shared_ptr<Api> api,
      std::shared_ptr<KeyStore> keystore,
      std::shared_ptr<PieceIO> piece_io)
      : host_{std::move(host)},
        context_{std::move(context)},
        api_{std::move(api)},
        keystore_{std::move(keystore)},
        piece_io_{std::move(piece_io)},
        network_{std::make_shared<Libp2pStorageMarketNetwork>(host_)},
        discovery_{std::make_shared<Discovery>(datastore)} {}

  void StorageMarketClientImpl::init() {
    std::shared_ptr<HostContext> fsm_context =
        std::make_shared<HostContextImpl>(context_);
    fsm_ = std::make_shared<ClientFSM>(makeFSMTransitions(), fsm_context);
  }

  void StorageMarketClientImpl::run() {}

  void StorageMarketClientImpl::stop() {}

  outcome::result<std::vector<StorageProviderInfo>>
  StorageMarketClientImpl::listProviders() const {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(miners, api_->StateListMiners(tipset_key));
    std::vector<StorageProviderInfo> storage_providers;
    for (const auto &miner_address : miners) {
      OUTCOME_TRY(miner_info, api_->StateMinerInfo(miner_address, tipset_key));
      OUTCOME_TRY(peer_id, PeerId::fromBytes(miner_info.peer_id));
      PeerInfo peer_info{.id = peer_id, .addresses = {}};
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
    return StorageMarketClientError::LOCAL_DEAL_NOT_FOUND;
  }

  void StorageMarketClientImpl::getAsk(
      const StorageProviderInfo &info,
      const SignedAskHandler &signed_ask_handler) const {
    network_->newAskStream(
        info.peer_info,
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
                                self->network_->closeStreamGracefully(stream);
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
      return StorageMarketClientError::PIECE_SIZE_GREATER_SECTOR_SIZE;
    }

    DealProposal deal_proposal{
        .piece_cid = comm_p,
        .piece_size = piece_size.padded(),
        .client = client_address,
        .provider = provider_info.address,
        .start_epoch = start_epoch,
        .end_epoch = end_epoch,
        .storage_price_per_epoch = price,
        .provider_collateral = static_cast<uint64_t>(piece_size),
        .client_collateral = 0};
    OUTCOME_TRY(signed_proposal, signProposal(client_address, deal_proposal));
    OUTCOME_TRY(proposal_cid, getProposalCid(signed_proposal));

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

    network_->newDealStream(
        provider_info.peer_info,
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

          self->connections_[proposal_cid] = stream.value();
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
        kMessageVersion,
        kStorageMarketAddress,
        address,
        {},
        amount,
        kGasPrice,
        kGasLimit,
        vm::actor::builtin::market::AddBalance::Number,
        {}};
    OUTCOME_TRY(signed_message, api_->MpoolPushMessage(unsigned_message));
    OUTCOME_TRY(message_cid, vm::message::cid(signed_message));
    OUTCOME_TRY(msg_wait, api_->StateWaitMsg(message_cid));
    OUTCOME_TRY(msg_state, msg_wait.waitSync());
    if (msg_state.receipt.exit_code != VMExitCode::Ok) {
      return StorageMarketClientError::ADD_FUNDS_CALL_ERROR;
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
      return StorageMarketClientError::WRONG_MINER;
    }
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(miner_info, api_->StateMinerInfo(info.address, tipset_key));
    OUTCOME_TRY(miner_key_address,
                api_->StateAccountKey(miner_info.worker, tipset_key));
    OUTCOME_TRY(ask_bytes, codec::cbor::encode(response.value().ask.ask));
    OUTCOME_TRY(
        signature_valid,
        keystore_->verify(
            miner_key_address, ask_bytes, response.value().ask.signature));
    if (!signature_valid) {
      logger_->debug("Ask response signature invalid");
      return StorageMarketClientError::SIGNATURE_INVALID;
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
      return StorageMarketClientError::PIECE_DATA_NOT_SET_MANUAL_TRANSFER;
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
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(miner_key_address,
                api_->StateAccountKey(deal->miner_worker, tipset_key));
    OUTCOME_TRY(response_bytes, codec::cbor::encode(response.response));
    OUTCOME_TRY(signature_valid,
                keystore_->verify(
                    miner_key_address, response_bytes, response.signature));
    if (!signature_valid) {
      return StorageMarketClientError::SIGNATURE_INVALID;
    }
    return outcome::success();
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
    // TODO wait for funding
  }

  void StorageMarketClientImpl::onClientEventFundsEnsured(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO handle if stream is absent
    auto stream = connections_[deal->proposal_cid];

    Proposal proposal{.deal_proposal = deal->client_deal_proposal,
                      .piece = deal->data_ref};
    stream->write(proposal,
                  [self{shared_from_this()}, deal, stream](
                      outcome::result<size_t> written) {
                    SELF_FSM_HALT_ON_ERROR(
                        written, "Send proposal error", deal);
                    self->network_->closeStreamGracefully(stream);
                    SELF_FSM_SEND(deal, ClientEvent::ClientEventDealProposed);
                  });
  }

  void StorageMarketClientImpl::onClientEventDealProposed(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO handle if stream is absent
    auto stream = connections_[deal->proposal_cid];
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
          != StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED) {
        // TODO handle reject reason
        SELF_FSM_SEND(deal, ClientEvent::ClientEventDealRejected);
        return;
      }
      self->network_->closeStreamGracefully(stream);
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
    // todo validate deal published
    OUTCOME_EXCEPT(fsm_->send(deal, ClientEvent::ClientEventDealPublished));
  }

  void StorageMarketClientImpl::onClientEventDealPublished(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // verify deal activated - on deal sector commit
    OUTCOME_EXCEPT(fsm_->send(deal, ClientEvent::ClientEventDealActivated));
  }

  void StorageMarketClientImpl::onClientEventDealActivated(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // final state
    // todo cleanup
  }

  void StorageMarketClientImpl::onClientEventFailed(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    std::stringstream ss;
    ss << "Proposal ";
    auto maybe_prpoposal_cid = deal->proposal_cid.toString();
    if (maybe_prpoposal_cid) ss << maybe_prpoposal_cid.value() << " ";
    ss << "failed. " << deal->message;
    logger_->error(ss.str());
  }

}  // namespace fc::markets::storage::client

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::client,
                            StorageMarketClientError,
                            e) {
  using fc::markets::storage::client::StorageMarketClientError;

  switch (e) {
    case StorageMarketClientError::WRONG_MINER:
      return "StorageMarketClientError: wrong miner address";
    case StorageMarketClientError::SIGNATURE_INVALID:
      return "StorageMarketClientError: signature invalid";
    case StorageMarketClientError::PIECE_DATA_NOT_SET_MANUAL_TRANSFER:
      return "StorageMarketClientError: piece data is not set for manual "
             "transfer";
    case StorageMarketClientError::PIECE_SIZE_GREATER_SECTOR_SIZE:
      return "StorageMarketClientError: piece size is greater sector size";
    case StorageMarketClientError::ADD_FUNDS_CALL_ERROR:
      return "StorageMarketClientError: add funds method call returned error";
    case StorageMarketClientError::LOCAL_DEAL_NOT_FOUND:
      return "StorageMarketClientError: local deal not found";
  }

  return "StorageMarketClientError: unknown error";
}
