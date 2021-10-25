/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/client/impl/storage_market_client_impl.hpp"

#include <boost/assert.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>

#include "codec/cbor/cbor_codec.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "common/outcome_fmt.hpp"
#include "common/ptr.hpp"
#include "markets/common.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/client/import_manager/import_manager.hpp"
#include "markets/storage/storage_datatransfer_voucher.hpp"
#include "storage/ipld/memory_indexed_car.hpp"
#include "vm/actor/builtin/types/market/publish_deals_result.hpp"
#include "vm/message/message.hpp"

#define MOVE(x)  \
  x {            \
    std::move(x) \
  }

#define CALLBACK_ACTION(_action)                                    \
  [this](auto deal, auto event, auto context, auto from, auto to) { \
    logger_->debug("Client FSM " #_action);                         \
    _action(deal, event, from, to);                                 \
    deal->state = to;                                               \
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
  constexpr size_t kProposeStreamOpenMax{20};
  constexpr size_t kStatusStreamOpenMax{20};

  using api::MsgWait;
  using libp2p::peer::PeerId;
  using primitives::BigInt;
  using primitives::sector::RegisteredSealProof;
  using vm::VMExitCode;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::builtin::v0::market::PublishStorageDeals;
  using vm::message::kDefaultGasLimit;
  using vm::message::kDefaultGasPrice;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  StorageMarketClientImpl::StorageMarketClientImpl(
      std::shared_ptr<Host> host,
      std::shared_ptr<boost::asio::io_context> context,
      std::shared_ptr<ImportManager> import_manager,
      std::shared_ptr<DataTransfer> datatransfer,
      std::shared_ptr<Discovery> discovery,
      std::shared_ptr<FullNodeApi> api,
      std::shared_ptr<ChainEvents> chain_events,
      std::shared_ptr<PieceIO> piece_io)
      : host_{std::move(host)},
        context_{std::move(context)},
        propose_streams_{
            std::make_shared<StreamOpenQueue>(host_, kProposeStreamOpenMax)},
        status_streams_{
            std::make_shared<StreamOpenQueue>(host_, kStatusStreamOpenMax)},
        api_{std::move(api)},
        chain_events_{std::move(chain_events)},
        piece_io_{std::move(piece_io)},
        discovery_{std::move(discovery)},
        import_manager_{std::move(import_manager)},
        datatransfer_{std::move(datatransfer)} {
    BOOST_ASSERT_MSG(api_, "StorageMarketClientImpl(): api is nullptr");
  }

  bool StorageMarketClientImpl::pollWaiting() {
    std::lock_guard lock{waiting_mutex};
    auto any{!waiting_deals.empty()};
    for (auto &deal : waiting_deals) {
      askDealStatus(deal);
    }
    waiting_deals.clear();
    return any;
  }

  void StorageMarketClientImpl::askDealStatus(
      std::shared_ptr<ClientDeal> deal) {
    auto cb{weakCb0(
        weak_from_this(), [=](outcome::result<DealStatusResponse> &&_res) {
          if (_res) {
            auto &res{_res.value()};
            auto state{res.state.status};
            if (state == StorageDealStatus::STORAGE_DEAL_STAGED
                || state == StorageDealStatus::STORAGE_DEAL_SEALING
                || state == StorageDealStatus::STORAGE_DEAL_ACTIVE
                || state == StorageDealStatus::STORAGE_DEAL_EXPIRED
                || state == StorageDealStatus::STORAGE_DEAL_SLASHED) {
              deal->publish_message = *res.state.publish_cid;
              FSM_SEND(deal, ClientEvent::ClientEventDealAccepted);
            } else if (state == StorageDealStatus::STORAGE_DEAL_FAILING
                       || state == StorageDealStatus::STORAGE_DEAL_ERROR) {
              deal->message =
                  "Got error deal status response: " + res.state.message;
              FSM_SEND(deal, ClientEvent::ClientEventDealRejected);
            } else {
              std::lock_guard lock{waiting_mutex};
              waiting_deals.push_back(deal);
            }
          } else {
            spdlog::error(
                "askDealStatus {} {}", deal->proposal_cid, _res.error());
            std::lock_guard lock{waiting_mutex};
            waiting_deals.push_back(deal);
          }
        })};
    DealStatusRequest req;
    req.proposal = deal->proposal_cid;
    OUTCOME_EXCEPT(bytes, codec::cbor::encode(req.proposal));
    OUTCOME_CB(
        req.signature,
        api_->WalletSign(deal->client_deal_proposal.proposal.client, bytes));
    status_streams_->open({
        deal->miner,
        kDealStatusProtocolId,
        [MOVE(cb), MOVE(req)](Host::StreamResult _stream) {
          OUTCOME_CB1(_stream);
          auto stream{std::make_shared<common::libp2p::CborStream>(
              std::move(_stream.value()))};
          stream->write(req, [MOVE(cb), stream](outcome::result<size_t> _n) {
            OUTCOME_CB1(_n);
            stream->read<DealStatusResponse>(
                [MOVE(cb), stream](outcome::result<DealStatusResponse> _res) {
                  cb(std::move(_res));
                });
          });
        },
    });
  }

  outcome::result<void> StorageMarketClientImpl::init() {
    // init fsm transitions
    fsm_ = std::make_shared<ClientFSM>(makeFSMTransitions(), *context_, false);
    return outcome::success();
  }

  void StorageMarketClientImpl::run() {}

  outcome::result<void> StorageMarketClientImpl::stop() {
    fsm_->stop();
    return outcome::success();
  }

  outcome::result<std::vector<StorageProviderInfo>>
  StorageMarketClientImpl::listProviders() const {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(miners, api_->StateListMiners(chain_head->key));
    std::vector<StorageProviderInfo> storage_providers;
    for (const auto &miner_address : miners) {
      OUTCOME_TRY(miner_info,
                  api_->StateMinerInfo(miner_address, chain_head->key));
      OUTCOME_TRY(peer_id, PeerId::fromBytes(miner_info.peer_id));
      PeerInfo peer_info{.id = std::move(peer_id), .addresses = {}};
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
    OUTCOME_TRY(all_deals, api_->StateMarketDeals(chain_head->key));
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
    host_->newStream(
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
          auto stream{std::make_shared<CborStream>(stream_res.value())};
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
      const RegisteredSealProof &registered_proof,
      bool verified_deal,
      bool is_fast_retrieval) {
    OUTCOME_TRY(comm_p_res, calculateCommP(registered_proof, data_ref));
    const auto &[comm_p, piece_size] = comm_p_res;
    if (piece_size.padded() > provider_info.sector_size) {
      return StorageMarketClientError::kPieceSizeGreaterSectorSize;
    }

    auto provider_collateral{collateral};
    if (provider_collateral.is_zero()) {
      OUTCOME_TRY(bounds,
                  api_->StateDealProviderCollateralBounds(
                      piece_size.padded(), verified_deal, {}));
      provider_collateral = bigdiv(bounds.min * 12, 10);
    }

    DealProposal deal_proposal{
        .piece_cid = comm_p,
        .piece_size = piece_size.padded(),
        .verified = verified_deal,
        .client = client_address,
        .provider = provider_info.address,
        .label = {},
        .start_epoch = start_epoch,
        .end_epoch = end_epoch,
        .storage_price_per_epoch = price,
        .provider_collateral = provider_collateral,
        .client_collateral = 0,
    };
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
                   .is_fast_retrieval = is_fast_retrieval,
                   .message = {},
                   .publish_message = {}});
    OUTCOME_TRY(
        fsm_->begin(client_deal, StorageDealStatus::STORAGE_DEAL_UNKNOWN));

    FSM_SEND(client_deal, ClientEvent::ClientEventOpen);

    OUTCOME_TRY(discovery_->addPeer(
        data_ref.root,
        {provider_info.address, provider_info.peer_info.id, comm_p}));

    return client_deal->proposal_cid;
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
    OUTCOME_TRY(miner_info,
                api_->StateMinerInfo(info.address, chain_head->key));
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
      const RegisteredSealProof &registered_proof,
      const DataRef &data_ref) const {
    if (data_ref.piece_cid.has_value()) {
      return std::pair(data_ref.piece_cid.value(), data_ref.piece_size);
    }
    if (data_ref.transfer_type == kTransferTypeManual) {
      return StorageMarketClientError::kPieceDataNotSetManualTransfer;
    }

    OUTCOME_TRY(car_file, import_manager_->get(data_ref.root));

    // TODO (a.chernyshov) selector builder
    // https://github.com/filecoin-project/go-fil-markets/blob/master/storagemarket/impl/clientutils/clientutils.go#L31
    return piece_io_->generatePieceCommitment(registered_proof,
                                              car_file.string());
  }

  outcome::result<ClientDealProposal> StorageMarketClientImpl::signProposal(
      const Address &address, const DealProposal &proposal) const {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(key_address, api_->StateAccountKey(address, chain_head->key));
    OUTCOME_TRY(proposal_bytes, codec::cbor::encode(proposal));
    OUTCOME_TRY(signature, api_->WalletSign(key_address, proposal_bytes));
    return ClientDealProposal{.proposal = proposal,
                              .client_signature = signature};
  }

  outcome::result<boost::optional<CID>> StorageMarketClientImpl::ensureFunds(
      std::shared_ptr<ClientDeal> deal) {
    OUTCOME_TRY(
        maybe_cid,
        api_->MarketReserveFunds(
            deal->client_deal_proposal.proposal.client,
            deal->client_deal_proposal.proposal.client,
            deal->client_deal_proposal.proposal.clientBalanceRequirement()));
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
      std::shared_ptr<ClientDeal> deal, api::MsgWait msg_state) {
    if (msg_state.receipt.exit_code != VMExitCode::kOk) {
      deal->message =
          "Publish deal exit code "
          + std::to_string(static_cast<uint64_t>(msg_state.receipt.exit_code));
      return false;
    }

    // check if published
    OUTCOME_TRY(publish_message, api_->ChainGetMessage(deal->publish_message));
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(
        miner_info,
        api_->StateMinerInfo(deal->client_deal_proposal.proposal.provider,
                             chain_head->key));
    OUTCOME_TRY(from_id_address,
                api_->StateLookupID(publish_message.from, chain_head->key));
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
    OUTCOME_TRY(params,
                codec::cbor::decode<PublishStorageDeals::Params>(
                    publish_message.params));
    auto &proposals{params.deals};
    auto it = std::find(
        proposals.begin(), proposals.end(), deal->client_deal_proposal);
    if (it == proposals.end()) {
      OUTCOME_TRY(proposal_cid_str, deal->proposal_cid.toString());
      deal->message = "deal publish didn't contain our deal (message cid: "
                      + proposal_cid_str + ")";
      return false;
    }

    // get proposal id from publish call return
    int index = std::distance(proposals.begin(), it);
    OUTCOME_TRY(network, api_->StateNetworkVersion(chain_head->key));
    OUTCOME_TRY(
        deal_id,
        vm::actor::builtin::types::market::publishDealsResult(
            msg_state.receipt.return_value, actorVersion(network), index));
    deal->deal_id = deal_id;
    return true;
  }

  void StorageMarketClientImpl::finalizeDeal(std::shared_ptr<ClientDeal> deal) {
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
                .to(StorageDealStatus::STORAGE_DEAL_VALIDATING)
                .action(CALLBACK_ACTION(onClientEventFundsEnsured)),
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
    auto maybe_funding_message_cid = ensureFunds(deal);
    if (maybe_funding_message_cid.has_error()) {
      deal->message =
          "Ensure funds failed: " + maybe_funding_message_cid.error().message();
      FSM_SEND(deal, ClientEvent::ClientEventFailed);
      return;
    }

    // funding message was sent
    if (maybe_funding_message_cid.value().has_value()) {
      deal->add_funds_cid = *maybe_funding_message_cid.value();
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
    logger_->info("onClientEventFundingInitiated StateWaitMsg {}",
                  *deal->add_funds_cid);
    api_->StateWaitMsg(
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
        },
        deal->add_funds_cid.get(),
        kMessageConfidence,
        api::kLookbackNoLimit,
        true);
  }

  void StorageMarketClientImpl::onClientEventFundsEnsured(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto cb{[self{shared_from_this()},
             deal](outcome::result<SignedResponse> response) {
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
      auto &res{response.value().response};
      if (res.state != StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED
          && res.state != StorageDealStatus::STORAGE_DEAL_WAITING_FOR_DATA) {
        deal->message = res.message;
        SELF_FSM_SEND(deal, ClientEvent::ClientEventDealRejected);
        return;
      }

      auto car_file = self->import_manager_->get(deal->data_ref.root);
      SELF_FSM_HALT_ON_ERROR(
          car_file,
          "Storage deal proposal error. Cannot get file from import manager",
          deal);
      auto _ipld{MemoryIndexedCar::make(car_file.value().string(), false)};
      SELF_FSM_HALT_ON_ERROR(_ipld, "MemoryIndexedCar::make", deal);
      auto &ipld{_ipld.value()};

      auto voucher =
          codec::cbor::encode(StorageDataTransferVoucher{deal->proposal_cid});
      SELF_FSM_HALT_ON_ERROR(
          voucher, "StorageDataTransferVoucher encoding", deal);

      if (deal->data_ref.transfer_type == kTransferTypeGraphsync) {
        self->datatransfer_->push(
            deal->miner,
            deal->data_ref.root,
            ipld,
            StorageDataTransferVoucherType,
            voucher.value(),
            [](auto) {},
            [](auto) {});
        // data transfer completed, the deal must be accepted
        self->askDealStatus(deal);
      } else if (deal->data_ref.transfer_type == kTransferTypeManual) {
        // wait for response in pollWaiting to check if deal is activated
        // or rejected
        std::lock_guard lock{self->waiting_mutex};
        self->waiting_deals.push_back(deal);
      } else {
        deal->message =
            "Wrong transfer type: '" + deal->data_ref.transfer_type + "'";
        SELF_FSM_SEND(deal, ClientEvent::ClientEventFailed);
      }
    }};
    propose(deal, std::move(cb));
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
    auto cb{[=](outcome::result<bool> verified) {
      FSM_HALT_ON_ERROR(verified, "Cannot get publish message", deal);
      if (!verified.value()) {
        FSM_SEND(deal, ClientEvent::ClientEventFailed);
        return;
      }
      FSM_SEND(deal, ClientEvent::ClientEventDealPublished);
    }};
    logger_->info("onClientEventDealAccepted StateWaitMsg {}",
                  deal->publish_message);
    api_->StateWaitMsg(
        [=, self{shared_from_this()}, cb{std::move(cb)}](auto _res) {
          OUTCOME_CB(auto res, _res);
          cb(verifyDealPublished(deal, res));
        },
        deal->publish_message,
        kMessageConfidence,
        api::kLookbackNoLimit,
        true);
  }

  void StorageMarketClientImpl::onClientEventDealPublished(
      std::shared_ptr<ClientDeal> deal,
      ClientEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    chain_events_->onDealSectorCommitted(
        deal->client_deal_proposal.proposal.provider,
        deal->deal_id,
        [self{shared_from_this()}, deal](auto _r) {
          SELF_FSM_HALT_ON_ERROR(_r, "onDealSectorCommitted error", deal);
          OUTCOME_EXCEPT(self->fsm_->send(
              deal, ClientEvent::ClientEventDealActivated, {}));
        });
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

  void StorageMarketClientImpl::propose(std::shared_ptr<ClientDeal> deal,
                                        ProposeCb cb) {
    propose_streams_->open({
        deal->miner,
        kDealProtocolId,
        [=, MOVE(deal), MOVE(cb)](Host::StreamResult _stream) {
          OUTCOME_CB1(_stream);
          auto stream{std::make_shared<CborStream>(std::move(_stream.value()))};
          Proposal proposal{
              deal->client_deal_proposal,
              deal->data_ref,
              deal->is_fast_retrieval,
          };
          stream->write(
              proposal, [=, MOVE(cb)](outcome::result<size_t> _write) {
                OUTCOME_CB1(_write);
                stream->read<SignedResponse>(
                    [MOVE(cb), stream](outcome::result<SignedResponse> _res) {
                      cb(std::move(_res));
                    });
              });
        },
    });
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
