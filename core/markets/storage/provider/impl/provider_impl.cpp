/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/impl/provider_impl.hpp"

#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "markets/storage/provider/storage_provider_error.hpp"
#include "markets/storage/provider/stored_ask.hpp"
#include "markets/storage/storage_datatransfer_voucher.hpp"
#include "markets/storage/types.hpp"
#include "storage/car/car.hpp"
#include "vm/actor/builtin/v0/market/market_actor.hpp"

#define CALLBACK_ACTION(_action)                                    \
  [this](auto deal, auto event, auto context, auto from, auto to) { \
    logger_->debug("Provider FSM " #_action);                       \
    _action(deal, event, from, to);                                 \
    deal->state = to;                                               \
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
  using data_transfer::Selector;
  using fc::storage::piece::DealInfo;
  using fc::storage::piece::PayloadLocation;
  using mining::SealingState;
  using vm::VMExitCode;
  using vm::actor::MethodParams;
  using vm::actor::builtin::v0::market::PublishStorageDeals;
  using vm::message::kDefaultGasLimit;
  using vm::message::kDefaultGasPrice;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  StorageProviderImpl::StorageProviderImpl(
      std::shared_ptr<Host> host,
      IpldPtr ipld,
      std::shared_ptr<DataTransfer> datatransfer,
      std::shared_ptr<StoredAsk> stored_ask,
      std::shared_ptr<boost::asio::io_context> context,
      std::shared_ptr<PieceStorage> piece_storage,
      std::shared_ptr<FullNodeApi> api,
      std::shared_ptr<SectorBlocks> sector_blocks,
      std::shared_ptr<ChainEvents> chain_events,
      const Address &miner_actor_address,
      std::shared_ptr<PieceIO> piece_io,
      std::shared_ptr<FileStore> filestore)
      : host_{std::move(host)},
        context_{std::move(context)},
        stored_ask_{std::move(stored_ask)},
        api_{std::move(api)},
        sector_blocks_{std::move(sector_blocks)},
        chain_events_{std::move(chain_events)},
        miner_actor_address_{miner_actor_address},
        piece_io_{std::move(piece_io)},
        piece_storage_{std::move(piece_storage)},
        filestore_{filestore},
        ipld_{ipld},
        datatransfer_{datatransfer} {}

  std::shared_ptr<MinerDeal> StorageProviderImpl::getDealPtr(
      const CID &proposal_cid) {
    for (auto &it : fsm_->list()) {
      if (it.first->proposal_cid == proposal_cid) {
        return it.first;
      }
    }
    return {};
  }

  outcome::result<void> StorageProviderImpl::init() {
    OUTCOME_TRY(
        filestore_->createDirectories(kStorageMarketImportDir.string()));

    serveAsk(*host_, stored_ask_);

    serveDealStatus(*host_, weak_from_this());

    host_->setProtocolHandler(
        kDealProtocolId, [self_wptr{weak_from_this()}](auto stream) {
          if (auto self = self_wptr.lock()) {
            self->handleDealStream(std::make_shared<CborStream>(stream));
          }
        });

    // init fsm transitions
    fsm_ =
        std::make_shared<ProviderFSM>(makeFSMTransitions(), *context_, false);

    datatransfer_->on_push.emplace(
        StorageDataTransferVoucherType,
        [this](auto &pdtid, auto &root, auto &, auto _voucher) {
          if (auto _voucher2{
                  codec::cbor::decode<StorageDataTransferVoucher>(_voucher)}) {
            if (auto deal{getDealPtr(_voucher2.value().proposal_cid)}) {
              return datatransfer_->acceptPush(
                  pdtid, root, [this, deal](auto ok) {
                    FSM_SEND(
                        deal,
                        ok ? ProviderEvent::ProviderEventDataTransferCompleted
                           : ProviderEvent::ProviderEventFailed);
                  });
            }
          }
          datatransfer_->rejectPush(pdtid);
        });

    return outcome::success();
  }

  outcome::result<void> StorageProviderImpl::start() {
    context_->post([self{shared_from_this()}] {
      self->logger_->debug(
          "Server started\nListening on: "
          + peerInfoToPrettyString(self->host_->getPeerInfo()));
    });

    return outcome::success();
  }

  outcome::result<void> StorageProviderImpl::stop() {
    fsm_->stop();
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto &[_, stream] : connections_) {
      closeStreamGracefully(stream, logger_);
    }
    return outcome::success();
  }

  outcome::result<MinerDeal> StorageProviderImpl::getDeal(
      const CID &proposal_cid) const {
    for (const auto &it : fsm_->list()) {
      if (it.first->proposal_cid == proposal_cid) {
        return *it.first;
      }
    }
    return StorageMarketProviderError::kLocalDealNotFound;
  }

  outcome::result<void> StorageProviderImpl::importDataForDeal(
      const CID &proposal_cid, const boost::filesystem::path &path) {
    auto fsm_state_table = fsm_->list();
    auto found_fsm_entity =
        std::find_if(fsm_state_table.begin(),
                     fsm_state_table.end(),
                     [proposal_cid](const auto &it) -> bool {
                       return it.first->proposal_cid == proposal_cid;
                     });
    if (found_fsm_entity == fsm_state_table.end()) {
      return StorageMarketProviderError::kLocalDealNotFound;
    }
    auto deal = found_fsm_entity->first;

    // copy imported file
    OUTCOME_TRY(cid_str, deal->ref.root.toString());
    auto car_path = kStorageMarketImportDir / cid_str;
    if (path != car_path)
      boost::filesystem::copy_file(
          path, car_path, boost::filesystem::copy_option::overwrite_if_exists);

    auto unpadded{proofs::padPiece(car_path)};
    if (unpadded.padded() != deal->client_deal_proposal.proposal.piece_size) {
      return StorageMarketProviderError::kPieceCIDDoesNotMatch;
    }
    OUTCOME_TRY(registered_proof, api_->GetProofType(miner_actor_address_, {}));
    OUTCOME_TRY(piece_commitment,
                piece_io_->generatePieceCommitment(registered_proof, car_path));

    if (piece_commitment.first
        != deal->client_deal_proposal.proposal.piece_cid) {
      return StorageMarketProviderError::kPieceCIDDoesNotMatch;
    }
    deal->piece_path = car_path.string();

    FSM_SEND(deal, ProviderEvent::ProviderEventVerifiedData);
    return outcome::success();
  }

  outcome::result<Signature> StorageProviderImpl::sign(const Bytes &input) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(worker_info,
                api_->StateMinerInfo(miner_actor_address_, chain_head->key));
    OUTCOME_TRY(worker_key_address,
                api_->StateAccountKey(worker_info.worker, chain_head->key));
    return api_->WalletSign(worker_key_address, input);
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
                        .is_fast_retrieval = proposal.value().is_fast_retrieval,
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
    auto proposal = deal->client_deal_proposal.proposal;
    OUTCOME_TRY(proposal_bytes, codec::cbor::encode(proposal));
    OUTCOME_TRY(
        verified,
        api_->WalletVerify(proposal.client,
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

    OUTCOME_TRY(chain_head, api_->ChainHead());
    if (chain_head->epoch()
        > proposal.start_epoch - kDefaultDealAcceptanceBuffer) {
      deal->message =
          "Deal proposal verification failed, deal start epoch is too soon or "
          "deal already expired";
      return false;
    }

    OUTCOME_TRY(ask, stored_ask_->getAsk(miner_actor_address_));
    auto min_price = bigdiv(
        ask.ask.price * static_cast<uint64_t>(proposal.piece_size), 1 << 30);
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
                api_->StateMarketBalance(proposal.client, chain_head->key));
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
    auto proposal = deal->client_deal_proposal.proposal;
    OUTCOME_TRY(worker_info,
                api_->StateMinerInfo(proposal.provider, chain_head->key));
    OUTCOME_TRY(maybe_cid,
                api_->MarketReserveFunds(worker_info.worker,
                                         proposal.provider,
                                         proposal.provider_collateral));
    return std::move(maybe_cid);
  }

  outcome::result<CID> StorageProviderImpl::publishDeal(
      const std::shared_ptr<MinerDeal> &deal) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(
        worker_info,
        api_->StateMinerInfo(deal->client_deal_proposal.proposal.provider,
                             chain_head->key));
    PublishStorageDeals::Params params{{deal->client_deal_proposal}};
    OUTCOME_TRY(encoded_params, codec::cbor::encode(params));
    UnsignedMessage unsigned_message(vm::actor::kStorageMarketAddress,
                                     worker_info.worker,
                                     0,
                                     TokenAmount{0},
                                     {},
                                     {},
                                     PublishStorageDeals::Number,
                                     MethodParams{encoded_params});
    OUTCOME_TRY(signed_message,
                api_->MpoolPushMessage(unsigned_message, api::kPushNoSpec));
    CID cid = signed_message.getCid();
    OUTCOME_TRY(str_cid, cid.toString());
    logger_->debug("Deal published with CID = " + str_cid);
    return std::move(cid);
  }

  outcome::result<void> StorageProviderImpl::sendSignedResponse(
      std::shared_ptr<MinerDeal> deal) {
    Response response{.state = deal->state,
                      .message = deal->message,
                      .proposal = deal->proposal_cid};
    OUTCOME_TRY(encoded_response, codec::cbor::encode(response));
    OUTCOME_TRY(signature, sign(encoded_response));
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
                    closeStreamGracefully(stream, self->logger_);
                  });

    return outcome::success();
  }

  outcome::result<PieceLocation> StorageProviderImpl::locatePiece(
      std::shared_ptr<MinerDeal> deal) {
    OUTCOME_TRY(piece_refs, sector_blocks_->getRefs(deal->deal_id));

    boost::optional<PieceLocation> piece_location;

    for (const auto &ref : piece_refs) {
      OUTCOME_TRY(sector_info,
                  sector_blocks_->getMiner()->getSectorInfo(ref.sector_number));

      if (sector_info->state == SealingState::kProving) {
        piece_location = ref;
        break;
      }
    }

    if (!piece_location.has_value()) {
      return StorageProviderError::kNotFoundSector;
    }

    return std::move(piece_location.get());
  }

  outcome::result<void> StorageProviderImpl::recordPieceInfo(
      std::shared_ptr<MinerDeal> deal, const PieceLocation &piece_location) {
    std::map<CID, PayloadLocation> locations;
    if (!deal->metadata_path.empty()) {
      // TODO (a.chernyshov) load block locations from metadata file
      // https://github.com/filecoin-project/go-fil-markets/blob/master/storagemarket/impl/providerstates/provider_states.go#L310
    } else {
      locations[deal->ref.root] = {};
    }
    OUTCOME_TRY(piece_storage_->addPayloadLocations(
        deal->client_deal_proposal.proposal.piece_cid, locations));
    OUTCOME_TRY(piece_storage_->addDealForPiece(
        deal->client_deal_proposal.proposal.piece_cid,
        DealInfo{.deal_id = deal->deal_id,
                 .sector_id = piece_location.sector_number,
                 .offset = piece_location.offset,
                 .length = piece_location.length}));
    return outcome::success();
  }

  outcome::result<std::shared_ptr<CborStream>> StorageProviderImpl::getStream(
      const CID &proposal_cid) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto stream_it = connections_.find(proposal_cid);
    if (stream_it == connections_.end()) {
      return StorageProviderError::kStreamLookupError;
    }
    return stream_it->second;
  }

  outcome::result<void> StorageProviderImpl::finalizeDeal(
      std::shared_ptr<MinerDeal> deal) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto stream_it = connections_.find(deal->proposal_cid);
    if (stream_it != connections_.end()) {
      closeStreamGracefully(stream_it->second, logger_);
      connections_.erase(stream_it);
    }
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
            .fromMany(StorageDealStatus::STORAGE_DEAL_WAITING_FOR_DATA,
                      StorageDealStatus::STORAGE_DEAL_TRANSFERRING)
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
            .to(StorageDealStatus::STORAGE_DEAL_EXPIRED)
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
    deal->state = StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED;
    FSM_HALT_ON_ERROR(
        sendSignedResponse(deal), "Error when sending response", deal);

    if (deal->ref.transfer_type == kTransferTypeManual) {
      deal->state = StorageDealStatus::STORAGE_DEAL_WAITING_FOR_DATA;
      FSM_SEND(deal, ProviderEvent::ProviderEventWaitingForManualData);
    } else if (deal->ref.transfer_type == kTransferTypeGraphsync) {
      FSM_SEND(deal, ProviderEvent::ProviderEventDataTransferInitiated);
    } else {
      deal->message = "Wrong transfer type: '" + deal->ref.transfer_type + "'";
      FSM_SEND(deal, ProviderEvent::ProviderEventFailed);
    }
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
    api_->StateWaitMsg(
        [self{shared_from_this()}, deal](outcome::result<MsgWait> result) {
          SELF_FSM_HALT_ON_ERROR(result, "Wait for funding error", deal);
          if (result.value().receipt.exit_code != VMExitCode::kOk) {
            deal->message = "Funding exit code "
                            + std::to_string(static_cast<uint64_t>(
                                result.value().receipt.exit_code));
            SELF_FSM_SEND(deal, ProviderEvent::ProviderEventFailed);
            return;
          }
          SELF_FSM_SEND(deal, ProviderEvent::ProviderEventFunded);
        },
        deal->add_funds_cid.get(),
        kMessageConfidence,
        api::kLookbackNoLimit,
        true);
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
    auto cid_str{deal->ref.root.toString()};
    FSM_HALT_ON_ERROR(cid_str, "CIDtoString", deal);
    auto car_path = kStorageMarketImportDir / cid_str.value();
    FSM_HALT_ON_ERROR(
        fc::storage::car::makeSelectiveCar(
            *ipld_, {{deal->ref.root, Selector{}}}, car_path.string()),
        "makeSelectiveCar",
        deal);
    FSM_HALT_ON_ERROR(importDataForDeal(deal->proposal_cid, car_path),
                      "importDataForDeal",
                      deal);
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
    api_->StateWaitMsg(
        [self{shared_from_this()}, deal, to](outcome::result<MsgWait> result) {
          SELF_FSM_HALT_ON_ERROR(
              result, "Publish storage deal message error", deal);
          if (result.value().receipt.exit_code != VMExitCode::kOk) {
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
          SELF_FSM_SEND(deal, ProviderEvent::ProviderEventDealPublished);
        },
        deal->publish_cid.get(),
        kMessageConfidence,
        api::kLookbackNoLimit,
        true);
  }

  void StorageProviderImpl::onProviderEventDealPublished(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    // TODO hand off
    auto &p{deal->client_deal_proposal.proposal};
    OUTCOME_EXCEPT(sector_blocks_->addPiece(
        p.piece_size.unpadded(),
        deal->piece_path,
        mining::types::DealInfo{deal->publish_cid,
                                deal->deal_id,
                                p,
                                {p.start_epoch, p.end_epoch},
                                deal->is_fast_retrieval}));
    FSM_SEND(deal, ProviderEvent::ProviderEventDealHandedOff);
  }

  void StorageProviderImpl::onProviderEventDealHandedOff(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    chain_events_->onDealSectorCommitted(
        deal->client_deal_proposal.proposal.provider,
        deal->deal_id,
        [=](auto _r) {
          FSM_HALT_ON_ERROR(_r, "onDealSectorCommitted error", deal);
          FSM_SEND(deal, ProviderEvent::ProviderEventDealActivated);
        });
  }

  void StorageProviderImpl::onProviderEventDealActivated(
      std::shared_ptr<MinerDeal> deal,
      ProviderEvent event,
      StorageDealStatus from,
      StorageDealStatus to) {
    auto maybe_piece_location = locatePiece(deal);
    FSM_HALT_ON_ERROR(maybe_piece_location, "Unable to locate piece", deal);
    FSM_HALT_ON_ERROR(recordPieceInfo(deal, maybe_piece_location.value()),
                      "Record piece failed",
                      deal);
    // TODO: wait expiration
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

  void serveAsk(libp2p::Host &host, std::weak_ptr<StoredAsk> _asker) {
    auto handle{[&](auto &&protocol) {
      host.setProtocolHandler(protocol, [_asker](auto _stream) {
        auto stream{std::make_shared<common::libp2p::CborStream>(_stream)};
        stream->template read<AskRequest>([_asker, stream](auto _request) {
          if (_request) {
            if (auto asker{_asker.lock()}) {
              if (auto _ask{asker->getAsk(_request.value().miner)}) {
                return stream->write(AskResponse{_ask.value()},
                                     [stream](auto) { stream->close(); });
              }
            }
          }
          stream->stream()->reset();
        });
      });
    }};
    handle(kAskProtocolId0);
    handle(kAskProtocolId);
  }

  void serveDealStatus(libp2p::Host &host,
                       std::weak_ptr<StorageProviderImpl> _provider) {
    auto handle{[&](auto &&protocol) {
      host.setProtocolHandler(protocol, [_provider](auto _stream) {
        auto stream{std::make_shared<common::libp2p::CborStream>(_stream)};
        stream->template read<DealStatusRequest>(
            [_provider, stream](auto _request) {
              if (_request) {
                auto &request{_request.value()};
                // TODO: check client signature
                if (auto provider{_provider.lock()}) {
                  if (auto _deal{provider->getDeal(request.proposal)}) {
                    auto &deal{_deal.value()};
                    DealStatusResponse response{
                        {
                            deal.state,
                            deal.message,
                            deal.client_deal_proposal.proposal,
                            deal.proposal_cid,
                            deal.add_funds_cid,
                            deal.publish_cid,
                            deal.deal_id,
                            deal.is_fast_retrieval,
                        },
                        {}};
                    OUTCOME_EXCEPT(input, codec::cbor::encode(response.state));
                    if (auto _sig{provider->sign(input)}) {
                      response.signature = std::move(_sig.value());
                      return stream->write(response,
                                           [stream](auto) { stream->close(); });
                    }
                  }
                }
              }
              stream->stream()->reset();
            });
      });
    }};
    handle(kDealStatusProtocolId);
  }
}  // namespace fc::markets::storage::provider

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::provider,
                            StorageMarketProviderError,
                            e) {
  using fc::markets::storage::provider::StorageMarketProviderError;

  switch (e) {
    case StorageMarketProviderError::kLocalDealNotFound:
      return "StorageMarketProviderError: local deal not found";
    case StorageMarketProviderError::kPieceCIDDoesNotMatch:
      return "StorageMarketProviderError: imported piece cid doensn't match "
             "proposal piece cid";
  }

  return "StorageMarketProviderError: unknown error";
}
