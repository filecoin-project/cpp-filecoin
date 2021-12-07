/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/provider/impl/retrieval_provider_impl.hpp"

#include <boost/filesystem.hpp>

#include "common/libp2p/peer/peer_info_helper.hpp"
#include "markets/storage/types.hpp"
#include "storage/piece/impl/piece_storage_error.hpp"

namespace fc::markets::retrieval::provider {
  using ::fc::storage::piece::PieceStorageError;
  using primitives::piece::UnpaddedByteIndex;
  using primitives::sector::SectorId;
  using primitives::sector::SectorRef;
  using storage::kStorageMarketImportDir;
  namespace fs = boost::filesystem;

  RetrievalProviderImpl::RetrievalProviderImpl(
      std::shared_ptr<Host> host,
      std::shared_ptr<DataTransfer> datatransfer,
      std::shared_ptr<api::FullNodeApi> api,
      std::shared_ptr<PieceStorage> piece_storage,
      std::shared_ptr<OneKey> config_key,
      std::shared_ptr<Manager> sealer,
      std::shared_ptr<Miner> miner)
      : host_{std::move(host)},
        datatransfer_{std::move(datatransfer)},
        api_{std::move(api)},
        piece_storage_{std::move(piece_storage)},
        config_key_{std::move(config_key)},
        config_{
            kDefaultPricePerByte,
            kDefaultUnsealPrice,
            kDefaultPaymentInterval,
            kDefaultPaymentIntervalIncrease,
        },
        sealer_{std::move(sealer)},
        miner_{std::move(miner)} {
    if (!config_key_->has()) {
      config_key_->setCbor(config_);
    }
    config_key_->getCbor(config_);
    datatransfer_->on_pull.emplace(
        DealProposalV0_0_1::type,
        [this](auto &pdtid, auto &pgsid, auto &, auto _voucher) {
          if (auto proposal{
                  codec::cbor::decode<DealProposalV0_0_1>(_voucher)}) {
            auto proposal_ptr =
                std::make_shared<DealProposalV0_0_1>(proposal.value());
            io_.io->post([=] { onProposal(pdtid, pgsid, proposal_ptr); });
            return;
          }
          datatransfer_->rejectPull(pdtid, pgsid, {}, {});
        });
    datatransfer_->on_pull.emplace(
        DealProposalV1_0_0::type,
        [this](auto &pdtid, auto &pgsid, auto &, auto _voucher) {
          if (auto proposal{
                  codec::cbor::decode<DealProposalV1_0_0>(_voucher)}) {
            auto proposal_ptr =
                std::make_shared<DealProposalV1_0_0>(proposal.value());
            io_.io->post([=] { onProposal(pdtid, pgsid, proposal_ptr); });
            return;
          }
          datatransfer_->rejectPull(pdtid, pgsid, {}, {});
        });
  }

  RetrievalAsk RetrievalProviderImpl::getAsk() const {
    return config_;
  }

  void RetrievalProviderImpl::setAsk(const RetrievalAsk &ask) {
    config_key_->setCbor(ask);
    config_ = ask;
  }

  void RetrievalProviderImpl::onProposal(
      const PeerDtId &pdtid,
      const PeerGsId &pgsid,
      const std::shared_ptr<DealProposal> &proposal) {
    if (proposal->params.price_per_byte < config_.price_per_byte
        || proposal->params.payment_interval > config_.payment_interval
        || proposal->params.payment_interval_increase
               > config_.interval_increase
        || proposal->params.unseal_price < config_.unseal_price) {
      rejectDatatransferPull(proposal->getType(),
                             pdtid,
                             pgsid,
                             proposal->deal_id,
                             DealStatus::kDealStatusRejected,
                             "Deal parameters not accepted");
      return;
    }

    auto _found{piece_storage_->hasPieceInfo(proposal->payload_cid,
                                             proposal->params.piece)};
    if (!_found || !_found.value()) {
      rejectDatatransferPull(proposal->getType(),
                             pdtid,
                             pgsid,
                             proposal->deal_id,
                             DealStatus::kDealStatusFailed,
                             "Payload not found");
      return;
    }

    auto deal{std::make_shared<DealState>(pdtid, pgsid, proposal)};
    auto &unseal{deal->state.owed};

    DealResponse deal_response{
        .status = unseal ? DealStatus::kDealStatusFundsNeededUnseal
                         : DealStatus::kDealStatusAccepted,
        .deal_id = proposal->deal_id,
        .payment_owed = unseal,
        .message = {},
    };
    acceptDatatransferPull(proposal->getType(), pdtid, pgsid, deal_response);

    datatransfer_->pulling_in.emplace(
        pdtid, [this, deal](auto &type, auto _voucher) {
          if (auto _payment{
                  codec::cbor::decode<DealPayment::Named>(_voucher)}) {
            io_.io->post([=] { onPayment(deal, _payment.value()); });
          } else {
            doFail(deal, _payment.error().message());
          }
        });

    if (!unseal) {
      doUnseal(deal);
    }
  }

  void RetrievalProviderImpl::onPayment(const std::shared_ptr<DealState> &deal,
                                        const DealPayment &payment) {
    auto _received{api_->PaychVoucherAdd(payment.payment_channel,
                                         payment.payment_voucher,
                                         {},
                                         deal->state.owed)};
    if (!_received) {
      return doFail(deal, _received.error().message());
    }
    auto &received{_received.value()};
    if (received == 0) {
      received = payment.payment_voucher.amount - deal->state.paid;
      if (received > deal->state.owed) {
        received = deal->state.owed;
      }
    }

    deal->state.pay(received);

    if (hasOwed(deal)) {
      return;
    }

    if (!deal->unsealed) {
      return doUnseal(deal);
    }
    if (!deal->traverser->isCompleted()) {
      return doBlocks(deal);
    }
    doComplete(deal);
  }

  void RetrievalProviderImpl::acceptDatatransferPull(
      const std::string &protocol,
      const PeerDtId &pdtid,
      const PeerGsId &pgsid,
      const DealResponse &deal_response) {
    if (protocol == DealProposalV0_0_1::type) {
      acceptDataTransferPoolWithResponse<DealResponseV0_0_1>(
          pdtid, pgsid, deal_response);
    } else if (protocol == DealProposalV1_0_0::type) {
      acceptDataTransferPoolWithResponse<DealResponseV1_0_0>(
          pdtid, pgsid, deal_response);
    }
  }

  void RetrievalProviderImpl::respondDataTransfer(
      const std::string &protocol,
      const PeerDtId &pdtid,
      const PeerGsId &pgsid,
      bool full_content,
      const DealResponse &deal_response) {
    if (protocol == DealProposalV0_0_1::type) {
      respondDataTransferWithResponse<DealResponseV0_0_1>(
          pdtid, pgsid, full_content, deal_response);
    } else if (protocol == DealProposalV1_0_0::type) {
      respondDataTransferWithResponse<DealResponseV1_0_0>(
          pdtid, pgsid, full_content, deal_response);
    }
  }

  void RetrievalProviderImpl::rejectDatatransferPull(
      const std::string &protocol,
      const PeerDtId &pdtid,
      const PeerGsId &pgsid,
      primitives::DealId deal_id,
      DealStatus status,
      const std::string &message) {
    if (protocol == DealProposalV0_0_1::type) {
      rejectDataTransferWithResponse<DealResponseV0_0_1>(
          pdtid, pgsid, deal_id, status, message);
    } else if (protocol == DealProposalV1_0_0::type) {
      rejectDataTransferWithResponse<DealResponseV1_0_0>(
          pdtid, pgsid, deal_id, status, message);
    }
  }

  void RetrievalProviderImpl::doUnseal(const std::shared_ptr<DealState> &deal) {
    if (hasOwed(deal)) {
      return;
    }
    auto _piece{piece_storage_->getPieceInfoFromCid(
        deal->proposal->payload_cid, deal->proposal->params.piece)};
    if (!_piece) {
      return doFail(deal, _piece.error().message());
    }
    if (!fs::exists(kStorageMarketImportDir)) {
      fs::create_directories(kStorageMarketImportDir);
    }
    // TODO(turuslan): deterministic name
    const auto car_path = kStorageMarketImportDir / fs::unique_path();
    auto _ = gsl::finally([&car_path]() {
      if (fs::exists(car_path)) {
        fs::remove_all(car_path);
      }
    });
    for (auto &info : _piece.value().deals) {
      if (auto _error{unsealSector(info.sector_id,
                                   info.offset.unpadded(),
                                   info.length.unpadded(),
                                   car_path.string())}) {
        assert(info.length.unpadded() == fs::file_size(car_path));
        auto _load{MemoryIndexedCar::make(car_path.string(), false)};
        if (!_load) {
          return doFail(deal, _load.error().message());
        }
        deal->ipld = _load.value();
        deal->traverser.emplace(Traverser{
            *deal->ipld,
            deal->proposal->payload_cid,
            deal->proposal->params.selector,
            false,
        });
        deal->unsealed = true;
        doBlocks(deal);
        return;
      }
    }
    doFail(deal, "unsealing all failed");
  }

  void RetrievalProviderImpl::doBlocks(const std::shared_ptr<DealState> &deal) {
    if (hasOwed(deal)) {
      return;
    }
    while (true) {
      auto _cid{deal->traverser->advance()};
      if (!_cid) {
        return doFail(deal, _cid.error().message());
      }
      auto &cid{_cid.value()};
      auto _data{deal->ipld->get(cid)};
      if (!_data) {
        return doFail(deal, _data.error().message());
      }
      auto &data{_data.value()};
      deal->state.block(data.size());
      datatransfer_->gs->postResponse(deal->pgsid,
                                      {GsResStatus::RS_PARTIAL_RESPONSE,
                                       {},
                                       {{std::move(cid), std::move(data)}}});

      if (deal->traverser->isCompleted()) {
        return doComplete(deal);
      }

      if (deal->state.owed) {
        return doBlocks(deal);
      }
    }
  }

  void RetrievalProviderImpl::doComplete(
      const std::shared_ptr<DealState> &deal) {
    deal->state.last();

    if (hasOwed(deal)) {
      return;
    }

    DealResponse deal_response{
        DealStatus::kDealStatusCompleted,
        deal->proposal->deal_id,
        {},
        {},
    };
    respondDataTransfer(deal->proposal->getType(),
                        deal->pdtid,
                        deal->pgsid,
                        true,
                        deal_response);
  }

  bool RetrievalProviderImpl::hasOwed(const std::shared_ptr<DealState> &deal) {
    if (!deal->state.owed) {
      return false;
    }
    DealResponse deal_response{
        .status = deal->unsealed
                      ? deal->traverser->isCompleted()
                            ? DealStatus::kDealStatusFundsNeededLastPayment
                            : DealStatus::kDealStatusFundsNeeded
                      : DealStatus::kDealStatusFundsNeededUnseal,
        .deal_id = deal->proposal->deal_id,
        .payment_owed = deal->state.owed,
        .message = {},
    };
    respondDataTransfer(deal->proposal->getType(),
                        deal->pdtid,
                        deal->pgsid,
                        false,
                        deal_response);
    return true;
  }

  void RetrievalProviderImpl::doFail(const std::shared_ptr<DealState> &deal,
                                     const std::string &error) {
    datatransfer_->pulling_in.erase(deal->pdtid);
    rejectDatatransferPull(deal->proposal->getType(),
                           deal->pdtid,
                           deal->pgsid,
                           deal->proposal->deal_id,
                           DealStatus::kDealStatusErrored,
                           error);
  }

  void RetrievalProviderImpl::start() {
    host_->setProtocolHandler(
        kQueryProtocolIdV0_0_1, [_self{weak_from_this()}](auto stream) {
          auto self{_self.lock()};
          if (!self) {
            return;
          }
          self->handleQuery<QueryRequestV0_0_1, QueryResponseV0_0_1>(
              std::make_shared<CborStream>(stream));
        });
    host_->setProtocolHandler(
        kQueryProtocolIdV1_0_0, [_self{weak_from_this()}](auto stream) {
          auto self{_self.lock()};
          if (!self) {
            return;
          }
          self->handleQuery<QueryRequestV1_0_0, QueryResponseV1_0_0>(
              std::make_shared<CborStream>(stream));
        });
    logger_->info("has been launched with ID "
                  + peerInfoToPrettyString(host_->getPeerInfo()));
  }

  void RetrievalProviderImpl::setPricePerByte(TokenAmount amount) {
    config_.price_per_byte = amount;
  }

  void RetrievalProviderImpl::setPaymentInterval(
      uint64_t payment_interval, uint64_t payment_interval_increase) {
    config_.payment_interval = payment_interval;
    config_.interval_increase = payment_interval_increase;
  }

  outcome::result<QueryResponse> RetrievalProviderImpl::makeQueryResponse(
      const QueryRequest &query) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(minfo,
                api_->StateMinerInfo(miner_->getAddress(), chain_head->key));

    OUTCOME_TRY(piece_available,
                piece_storage_->hasPieceInfo(query.payload_cid,
                                             query.params.piece_cid));
    if (!piece_available) {
      return QueryResponse{
          .response_status = QueryResponseStatus::kQueryResponseUnavailable,
          .item_status = QueryItemStatus::kQueryItemUnavailable};
    }
    OUTCOME_TRY(piece_size,
                piece_storage_->getPieceSize(query.payload_cid,
                                             query.params.piece_cid));
    return QueryResponse{
        .response_status = QueryResponseStatus::kQueryResponseAvailable,
        .item_status = QueryItemStatus::kQueryItemAvailable,
        .item_size = piece_size,
        .payment_address = minfo.worker,
        .min_price_per_byte = config_.price_per_byte,
        .payment_interval = config_.payment_interval,
        .interval_increase = config_.interval_increase};
  }

  outcome::result<void> RetrievalProviderImpl::unsealSector(
      SectorNumber sid,
      UnpaddedPieceSize offset,
      UnpaddedPieceSize size,
      const std::string &output_path) {
    OUTCOME_TRY(sector_info, miner_->getSectorInfo(sid));

    auto miner_id = miner_->getAddress().getId();

    SectorId sector_id{
        .miner = miner_id,
        .sector = sid,
    };
    SectorRef sector{
        .id = sector_id,
        .proof_type = sector_info->sector_type,
    };

    CID comm_d = sector_info->comm_d.get_value_or(CID());

    OUTCOME_TRY(
        sealer_->readPieceSync(PieceData(output_path, O_WRONLY | O_CREAT),
                               sector,
                               UnpaddedByteIndex(offset),
                               size,
                               sector_info->ticket,
                               comm_d));

    return outcome::success();
  }

}  // namespace fc::markets::retrieval::provider
