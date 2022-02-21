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
#include "vm/actor/builtin/types/market/publish_deals_result.hpp"

#define CALLBACK_ACTION(_action)                                            \
  [this](auto deal_context, auto event, auto context, auto from, auto to) { \
    logger_->debug("Provider FSM " #_action);                               \
    _action(deal_context, event, from, to);                                 \
    deal_context->deal->state = to;                                         \
  }

#define FSM_HALT_ON_ERROR(result, msg, deal_context)              \
  if ((result).has_error()) {                                     \
    (deal_context)->deal->message =                               \
        (msg) + std::string(". ") + (result).error().message();   \
    FSM_SEND((deal_context), ProviderEvent::ProviderEventFailed); \
    return;                                                       \
  }

#define SELF_FSM_HALT_ON_ERROR(result, msg, deal_context)              \
  if ((result).has_error()) {                                          \
    (deal_context)->deal->message =                                    \
        (msg) + std::string(". ") + (result).error().message();        \
    SELF_FSM_SEND((deal_context), ProviderEvent::ProviderEventFailed); \
    return;                                                            \
  }

namespace fc::markets::storage::provider {
  using data_transfer::Selector;
  using fc::storage::piece::DealInfo;
  using fc::storage::piece::PayloadLocation;
  using mining::SealingState;
  using vm::VMExitCode;
  using vm::actor::MethodParams;
  using vm::actor::builtin::v0::market::PublishStorageDeals;
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
      Address miner_actor_address,
      std::shared_ptr<PieceIO> piece_io,
      std::shared_ptr<FileStore> filestore,
      std::shared_ptr<DealInfoManager> deal_info_manager)
      : host_{std::move(host)},
        context_{std::move(context)},
        stored_ask_{std::move(stored_ask)},
        api_{std::move(api)},
        sector_blocks_{std::move(sector_blocks)},
        chain_events_{std::move(chain_events)},
        miner_actor_address_{std::move(miner_actor_address)},
        piece_io_{std::move(piece_io)},
        piece_storage_{std::move(piece_storage)},
        filestore_{std::move(filestore)},
        ipld_{std::move(ipld)},
        datatransfer_{std::move(datatransfer)},
        deal_info_manager_{std::move(deal_info_manager)} {}

  std::shared_ptr<StorageProviderImpl::DealContext>
  StorageProviderImpl::getDealContextPtr(const CID &proposal_cid) const {
    for (auto &it : fsm_->list()) {
      if (it.first->deal->proposal_cid == proposal_cid) {
        return it.first;
      }
    }
    return {};
  }

  outcome::result<void> StorageProviderImpl::init() {
    OUTCOME_TRY(
        filestore_->createDirectories(kStorageMarketImportDir.string()));

    // kAskProtocolId_v1_0_1 is not supported since stored ask stores only
    // v1_1_0
    setAskHandler<AskRequestV1_1_0, AskResponseV1_1_0>(kAskProtocolId_v1_1_0);

    setDealStatusHandler<DealStatusRequestV1_0_1, DealStatusResponseV1_0_1>(
        kDealStatusProtocolId_v1_0_1);
    setDealStatusHandler<DealStatusRequestV1_1_0, DealStatusResponseV1_1_0>(
        kDealStatusProtocolId_v1_1_0);

    setDealMkHandler<ProposalV1_0_1>(kDealMkProtocolId_v1_0_1);
    setDealMkHandler<ProposalV1_1_0>(kDealMkProtocolId_v1_1_0);

    // init fsm transitions
    fsm_ =
        std::make_shared<ProviderFSM>(makeFSMTransitions(), *context_, false);

    datatransfer_->on_push.emplace(
        StorageDataTransferVoucherType,
        [this](auto &pdtid, auto &root, auto &, auto _voucher) {
          if (auto _voucher2{
                  codec::cbor::decode<StorageDataTransferVoucher>(_voucher)}) {
            if (auto deal_context{
                    getDealContextPtr(_voucher2.value().proposal_cid)}) {
              return datatransfer_->acceptPush(
                  pdtid, root, [this, deal_context](auto ok) {
                    FSM_SEND(
                        deal_context,
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
      if (it.first->deal->proposal_cid == proposal_cid) {
        return *it.first->deal;
      }
    }
    return StorageMarketProviderError::kLocalDealNotFound;
  }

  outcome::result<std::vector<MinerDeal>> StorageProviderImpl::getLocalDeals()
      const {
    std::vector<MinerDeal> deals;
    const auto fsm_deals = fsm_->list();
    deals.reserve(fsm_deals.size());
    for (const auto &it : fsm_deals) {
      deals.push_back(*it.first->deal);
    }

    return deals;
  }

  outcome::result<void> StorageProviderImpl::importDataForDeal(
      const CID &proposal_cid, const boost::filesystem::path &path) {
    const auto deal_context = getDealContextPtr(proposal_cid);

    // copy imported file
    OUTCOME_TRY(cid_str, deal_context->deal->ref.root.toString());
    auto car_path = kStorageMarketImportDir / cid_str;
    if (path != car_path)
      boost::filesystem::copy_file(
          path, car_path, boost::filesystem::copy_option::overwrite_if_exists);

    auto unpadded{proofs::padPiece(car_path)};
    if (unpadded.padded()
        != deal_context->deal->client_deal_proposal.proposal.piece_size) {
      return StorageMarketProviderError::kPieceCIDDoesNotMatch;
    }
    OUTCOME_TRY(registered_proof, api_->GetProofType(miner_actor_address_, {}));
    OUTCOME_TRY(piece_commitment,
                piece_io_->generatePieceCommitment(registered_proof, car_path));

    if (piece_commitment.first
        != deal_context->deal->client_deal_proposal.proposal.piece_cid) {
      return StorageMarketProviderError::kPieceCIDDoesNotMatch;
    }
    deal_context->deal->piece_path = car_path.string();

    FSM_SEND(deal_context, ProviderEvent::ProviderEventVerifiedData);
    return outcome::success();
  }

  outcome::result<Signature> StorageProviderImpl::sign(
      const Bytes &input) const {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(worker_info,
                api_->StateMinerInfo(miner_actor_address_, chain_head->key));
    OUTCOME_TRY(worker_key_address,
                api_->StateAccountKey(worker_info.worker, chain_head->key));
    return api_->WalletSign(worker_key_address, input);
  }

  outcome::result<MinerDeal> StorageProviderImpl::handleDealStatus(
      const DealStatusRequest &request) const {
    OUTCOME_TRY(deal, getDeal(request.proposal));

    // verify client's signature
    OUTCOME_TRY(bytes, request.getDigest());
    const auto &client_address = deal.client_deal_proposal.proposal.client;
    OUTCOME_TRY(verified,
                api_->WalletVerify(client_address, bytes, request.signature));
    if (!verified) {
      return ERROR_TEXT("Wrong request signature");
    }

    return std::move(deal);
  }

  void StorageProviderImpl::handleMKDealStream(
      const std::string &protocol,
      const std::shared_ptr<CborStream> &stream,
      const Proposal &proposal) {
    auto proposal_cid{proposal.deal_proposal.cid()};
    auto remote_peer_id = stream->stream()->remotePeerId();
    if (!hasValue(remote_peer_id, "Cannot get remote peer info: ", stream))
      return;
    auto remote_multiaddress = stream->stream()->remoteMultiaddr();
    if (!hasValue(remote_multiaddress, "Cannot get remote peer info: ", stream))
      return;
    PeerInfo remote_peer_info{.id = remote_peer_id.value(),
                              .addresses = {remote_multiaddress.value()}};
    std::shared_ptr<MinerDeal> deal = std::make_shared<MinerDeal>(
        MinerDeal{.client_deal_proposal = proposal.deal_proposal,
                  .proposal_cid = proposal_cid,
                  .add_funds_cid = boost::none,
                  .publish_cid = boost::none,
                  .client = remote_peer_info,
                  .state = StorageDealStatus::STORAGE_DEAL_UNKNOWN,
                  .piece_path = {},
                  .metadata_path = {},
                  .is_fast_retrieval = proposal.is_fast_retrieval,
                  .message = {},
                  .ref = proposal.piece,
                  .deal_id = {}});
    auto deal_context =
        std::make_shared<DealContext>(DealContext{deal, protocol});
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.emplace(proposal_cid, stream);
    OUTCOME_EXCEPT(
        fsm_->begin(deal_context, StorageDealStatus::STORAGE_DEAL_UNKNOWN));
    FSM_SEND(deal_context, ProviderEvent::ProviderEventOpen);
  }

  outcome::result<bool> StorageProviderImpl::verifyDealProposal(
      const std::shared_ptr<MinerDeal> &deal) const {
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
          "Deal proposal verification failed, deal start epoch is too soon "
          "or deal already expired";
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

    // This doesn't guarantee that the client won't withdraw / lock those
    // funds but it's a decent first filter
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
  StorageProviderImpl::ensureProviderFunds(
      const std::shared_ptr<MinerDeal> &deal) {
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
      const std::shared_ptr<DealContext> &deal_context) {
    Response response{.state = deal_context->deal->state,
                      .message = deal_context->deal->message,
                      .proposal = deal_context->deal->proposal_cid};
    OUTCOME_TRY(stream, getStream(deal_context->deal->proposal_cid));
    auto send_cb = [self{shared_from_this()},
                    stream](outcome::result<size_t> maybe_res) {
      if (maybe_res.has_error()) {
        // assume client disconnected
        self->logger_->error("Write deal response error. "
                             + maybe_res.error().message());
        return;
      }
      closeStreamGracefully(stream, self->logger_);
    };

    if (deal_context->protocol == kDealMkProtocolId_v1_0_1) {
      SignedResponseV1_0_1 signed_response(response);
      OUTCOME_TRY(digest, signed_response.getDigest());
      OUTCOME_TRYA(signed_response.signature, sign(digest));
      stream->write(signed_response, send_cb);
    } else if (deal_context->protocol == kDealMkProtocolId_v1_1_0) {
      SignedResponseV1_1_0 signed_response(response);
      OUTCOME_TRY(digest, signed_response.getDigest());
      OUTCOME_TRYA(signed_response.signature, sign(digest));
      stream->write(signed_response, send_cb);
    }

    return outcome::success();
  }

  outcome::result<PieceLocation> StorageProviderImpl::locatePiece(
      const std::shared_ptr<MinerDeal> &deal) {
    OUTCOME_TRY(piece_refs, sector_blocks_->getRefs(deal->deal_id));

    boost::optional<PieceLocation> piece_location;

    for (const auto &ref : piece_refs) {
      OUTCOME_TRY(sector_info,
                  sector_blocks_->getMiner()->getSectorInfo(ref.sector));

      if (sector_info->state == SealingState::kProving) {
        piece_location = ref;
        break;
      }
    }

    if (piece_location.has_value()) {
      return piece_location.get();
    }

    return StorageProviderError::kNotFoundSector;
  }

  outcome::result<void> StorageProviderImpl::recordPieceInfo(
      const std::shared_ptr<MinerDeal> &deal,
      const PieceLocation &piece_location) {
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
                 .sector_id = piece_location.sector,
                 .offset = piece_location.offset,
                 .length = piece_location.size}));
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
      const std::shared_ptr<MinerDeal> &deal) {
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

  std::vector<StorageProviderImpl::ProviderTransition>
  StorageProviderImpl::makeFSMTransitions() {
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

  FSM_HANDLE_DEFINITION(StorageProviderImpl::onProviderEventOpen) {
    auto verified = verifyDealProposal(deal_context->deal);
    FSM_HALT_ON_ERROR(verified, "Deal proposal verify error", deal_context);
    if (!verified.value()) {
      FSM_SEND(deal_context, ProviderEvent::ProviderEventFailed);
      return;
    }
    FSM_SEND(deal_context, ProviderEvent::ProviderEventDealAccepted);
  }

  FSM_HANDLE_DEFINITION(StorageProviderImpl::onProviderEventDealAccepted) {
    deal_context->deal->state =
        StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED;
    FSM_HALT_ON_ERROR(sendSignedResponse(deal_context),
                      "Error when sending response",
                      deal_context);

    if (deal_context->deal->ref.transfer_type == kTransferTypeManual) {
      deal_context->deal->state =
          StorageDealStatus::STORAGE_DEAL_WAITING_FOR_DATA;
      FSM_SEND(deal_context, ProviderEvent::ProviderEventWaitingForManualData);
    } else if (deal_context->deal->ref.transfer_type
               == kTransferTypeGraphsync) {
      FSM_SEND(deal_context, ProviderEvent::ProviderEventDataTransferInitiated);
    } else {
      deal_context->deal->message = "Wrong transfer type: '"
                                    + deal_context->deal->ref.transfer_type
                                    + "'";
      FSM_SEND(deal_context, ProviderEvent::ProviderEventFailed);
    }
  }

  FSM_HANDLE_DEFINITION(
      StorageProviderImpl::onProviderEventWaitingForManualData) {
    logger_->debug("Waiting for importDataForDeal() call");
  }

  FSM_HANDLE_DEFINITION(StorageProviderImpl::onProviderEventFundingInitiated) {
    api_->StateWaitMsg(
        [self{shared_from_this()},
         deal_context](outcome::result<MsgWait> result) {
          SELF_FSM_HALT_ON_ERROR(
              result, "Wait for funding error", deal_context);
          if (result.value().receipt.exit_code != VMExitCode::kOk) {
            deal_context->deal->message =
                "Funding exit code "
                + std::to_string(
                    static_cast<uint64_t>(result.value().receipt.exit_code));
            SELF_FSM_SEND(deal_context, ProviderEvent::ProviderEventFailed);
            return;
          }
          SELF_FSM_SEND(deal_context, ProviderEvent::ProviderEventFunded);
        },
        deal_context->deal->add_funds_cid.get(),
        kMessageConfidence,
        api::kLookbackNoLimit,
        true);
  }

  FSM_HANDLE_DEFINITION(StorageProviderImpl::onProviderEventFunded) {
    auto maybe_cid = publishDeal(deal_context->deal);
    FSM_HALT_ON_ERROR(maybe_cid, "Publish deal error", deal_context);
    deal_context->deal->publish_cid = maybe_cid.value();
    FSM_SEND(deal_context, ProviderEvent::ProviderEventDealPublishInitiated);
  }

  FSM_HANDLE_DEFINITION(
      StorageProviderImpl::onProviderEventDataTransferInitiated) {
    // nothing, wait for data transfer completed
  }

  FSM_HANDLE_DEFINITION(
      StorageProviderImpl::onProviderEventDataTransferCompleted) {
    auto cid_str{deal_context->deal->ref.root.toString()};
    FSM_HALT_ON_ERROR(cid_str, "CIDtoString", deal_context);
    auto car_path = kStorageMarketImportDir / cid_str.value();
    FSM_HALT_ON_ERROR(fc::storage::car::makeSelectiveCar(
                          *ipld_,
                          {{deal_context->deal->ref.root, Selector{}}},
                          car_path.string()),
                      "makeSelectiveCar",
                      deal_context);
    FSM_HALT_ON_ERROR(
        importDataForDeal(deal_context->deal->proposal_cid, car_path),
        "importDataForDeal",
        deal_context);
  }

  FSM_HANDLE_DEFINITION(StorageProviderImpl::onProviderEventVerifiedData) {
    auto funding_cid = ensureProviderFunds(deal_context->deal);
    FSM_HALT_ON_ERROR(
        funding_cid, "Ensure provider funds failed", deal_context);

    // funding message was sent
    if (funding_cid.value().has_value()) {
      deal_context->deal->add_funds_cid = *funding_cid.value();
      FSM_SEND(deal_context, ProviderEvent::ProviderEventFundingInitiated);
      return;
    }

    FSM_SEND(deal_context, ProviderEvent::ProviderEventFunded);
  }

  FSM_HANDLE_DEFINITION(
      StorageProviderImpl::onProviderEventDealPublishInitiated) {
    assert(deal_context->deal->publish_cid.has_value());

    api_->StateWaitMsg(
        [self{shared_from_this()}, deal_context, to](
            outcome::result<MsgWait> msg_state) {
          const auto maybe_deal_id =
              self->deal_info_manager_->dealIdFromPublishDealsMsg(
                  msg_state.value(),
                  deal_context->deal->client_deal_proposal.proposal);
          SELF_FSM_HALT_ON_ERROR(
              maybe_deal_id, "Looking for publish deal message", deal_context);
          deal_context->deal->deal_id = maybe_deal_id.value();
          deal_context->deal->state = to;
          SELF_FSM_SEND(deal_context,
                        ProviderEvent::ProviderEventDealPublished);
        },
        deal_context->deal->publish_cid.get(),
        // Wait for deal to be published (plus additional time for confidence)
        kMessageConfidence * 2,
        api::kLookbackNoLimit,
        true);
  }

  FSM_HANDLE_DEFINITION(StorageProviderImpl::onProviderEventDealPublished) {
    auto &proposal{deal_context->deal->client_deal_proposal.proposal};
    auto maybe_piece_location = sector_blocks_->addPiece(
        proposal.piece_size.unpadded(),
        deal_context->deal->piece_path,
        mining::types::DealInfo{deal_context->deal->publish_cid,
                                deal_context->deal->deal_id,
                                proposal,
                                {proposal.start_epoch, proposal.end_epoch},
                                deal_context->deal->is_fast_retrieval});
    FSM_HALT_ON_ERROR(
        maybe_piece_location, "Unable to locate piece", deal_context);
    deal_context->maybe_piece_location = maybe_piece_location.value();
    // TODO(a.chernyshov): add piece retry
    FSM_SEND(deal_context, ProviderEvent::ProviderEventDealHandedOff);
  }

  FSM_HANDLE_DEFINITION(StorageProviderImpl::onProviderEventDealHandedOff) {
    chain_events_->onDealSectorCommitted(
        deal_context->deal->client_deal_proposal.proposal.provider,
        deal_context->deal->deal_id,
        [=](auto _r) {
          FSM_HALT_ON_ERROR(_r, "onDealSectorCommitted error", deal_context);
          FSM_SEND(deal_context, ProviderEvent::ProviderEventDealActivated);
        });
  }

  FSM_HANDLE_DEFINITION(StorageProviderImpl::onProviderEventDealActivated) {
    if (not deal_context->maybe_piece_location.has_value()) {
      (deal_context)->deal->message = "Unknown piece location";
      FSM_SEND((deal_context), ProviderEvent::ProviderEventFailed);
      return;
    }

    FSM_HALT_ON_ERROR(
        recordPieceInfo(deal_context->deal,
                        deal_context->maybe_piece_location.value()),
        "Record piece failed",
        deal_context);
    // TODO(a.chernyshov): wait expiration
  }

  FSM_HANDLE_DEFINITION(StorageProviderImpl::onProviderEventDealCompleted) {
    logger_->debug("Deal completed");
    auto res = finalizeDeal(deal_context->deal);
    if (res.has_error()) {
      logger_->error("Deal finalization error. " + res.error().message());
    }
  }

  FSM_HANDLE_DEFINITION(StorageProviderImpl::onProviderEventFailed) {
    logger_->error("Deal failed with message: " + deal_context->deal->message);
    deal_context->deal->state = to;
    auto response_res = sendSignedResponse(deal_context);
    if (response_res.has_error()) {
      logger_->error("Error when sending error response. "
                     + response_res.error().message());
    }
    auto res = finalizeDeal(deal_context->deal);
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
    case StorageMarketProviderError::kLocalDealNotFound:
      return "StorageMarketProviderError: local deal not found";
    case StorageMarketProviderError::kPieceCIDDoesNotMatch:
      return "StorageMarketProviderError: imported piece cid doensn't match "
             "proposal piece cid";
  }

  return "StorageMarketProviderError: unknown error";
}
