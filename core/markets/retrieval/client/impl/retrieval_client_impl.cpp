/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/client/impl/retrieval_client_impl.hpp"

#include "codec/cbor/cbor_codec.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"

namespace fc::markets::retrieval::client {

  RetrievalClientImpl::RetrievalClientImpl(
      std::shared_ptr<Host> host,
      std::shared_ptr<DataTransfer> datatransfer,
      std::shared_ptr<FullNodeApi> api,
      std::shared_ptr<IpfsDatastore> ipfs)
      : host_{std::move(host)},
        datatransfer_{std::move(datatransfer)},
        api_{std::move(api)},
        ipfs_{std::move(ipfs)} {}

  outcome::result<std::vector<PeerInfo>> RetrievalClientImpl::findProviders(
      const CID &piece_cid) const {
    // TODO (a.chernyshov) implement
    return outcome::failure(RetrievalClientError::kUnknownResponseReceived);
  }

  void RetrievalClientImpl::query(const RetrievalPeer &provider_peer,
                                  const QueryRequest &request,
                                  const QueryResponseHandler &cb) {
    OUTCOME_CB(auto peer, getPeerInfo(provider_peer));
    host_->newStream(
        peer, kQueryProtocolId, [=, self{shared_from_this()}](auto stream_res) {
          OUTCOME_CB1(stream_res);
          self->logger_->debug("connected to provider ID "
                               + peerInfoToPrettyString(peer));
          auto stream{std::make_shared<CborStream>(stream_res.value())};
          stream->write(request, [=](auto written_res) {
            if (written_res.has_error()) {
              stream->close();
              cb(written_res.error());
              return;
            }
            stream->template read<QueryResponse>([=](auto response) {
              stream->close();
              cb(std::move(response));
            });
          });
        });
  }

  outcome::result<PeerInfo> RetrievalClientImpl::getPeerInfo(
      const RetrievalPeer &provider_peer) {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(miner_info,
                api_->StateMinerInfo(provider_peer.address, chain_head->key));
    return PeerInfo{provider_peer.peer_id, miner_info.multiaddrs};
  }

  outcome::result<void> RetrievalClientImpl::retrieve(
      const CID &payload_cid,
      const DealProposalParams &deal_params,
      const TokenAmount &total_funds,
      const RetrievalPeer &provider_peer,
      const Address &client_wallet,
      const Address &miner_wallet,
      const RetrieveResponseHandler &handler) {
    DealProposal::Named proposal{{.payload_cid = payload_cid,
                                  .deal_id = next_deal_id++,
                                  .params = deal_params}};
    auto deal{std::make_shared<DealState>(
        proposal, handler, client_wallet, miner_wallet, total_funds)};
    OUTCOME_TRY(peer_info, getPeerInfo(provider_peer));
    deal->pdtid = datatransfer_->pull(
        peer_info,
        payload_cid,
        deal_params.selector,
        DealProposal::Named::type,
        codec::cbor::encode(proposal).value(),
        [this, deal](auto &type, auto _voucher) {
          OUTCOME_EXCEPT(res,
                         codec::cbor::decode<DealResponse::Named>(_voucher));
          auto after{[=](auto &res) {
            if (res.status == DealStatus::kDealStatusCompleted) {
              deal->handler(outcome::success());
              datatransfer_->pulling_out.erase(deal->pdtid);
              return;
            }
            if (res.payment_owed) {
              processPaymentRequest(deal, deal->state.owed);
            }
          }};
          if (!deal->accepted) {
            std::unique_lock lock{deal->pending_mutex};
            deal->pending.emplace();
            lock.unlock();
            auto unseal{res.status == DealStatus::kDealStatusFundsNeededUnseal};
            const auto last{res.status
                            == DealStatus::kDealStatusFundsNeededLastPayment};
            deal->accepted =
                unseal || res.status == DealStatus::kDealStatusAccepted || last;
            if (!deal->accepted) {
              deal->handler(
                  res.status == DealStatus::kDealStatusRejected
                      ? RetrievalClientError::kResponseDealRejected
                  : res.status == DealStatus::kDealStatusDealNotFound
                      ? RetrievalClientError::kResponseNotFound
                      : RetrievalClientError::kUnknownResponseReceived);
              datatransfer_->pulling_out.erase(deal->pdtid);
              return;
            }
            auto _paych{api_->PaychGet(
                deal->client_wallet, deal->miner_wallet, deal->total_funds)};
            if (!_paych) {
              return deal->handler(_paych.error());
            }
            return _paych.value().waitOwn([=](auto &&_paych) {
              if (!_paych) {
                return deal->handler(_paych.error());
              }
              auto &paych{_paych.value().channel};
              deal->payment_channel_address = paych;
              auto _lane{api_->PaychAllocateLane(paych)};
              if (!_lane) {
                return deal->handler(_lane.error());
              }
              deal->lane_id = _lane.value();
              std::unique_lock lock{deal->pending_mutex};
              after(res);
              for (auto &res : *deal->pending) {
                after(res);
              }
              deal->pending.reset();
            });
          }
          std::unique_lock lock{deal->pending_mutex};
          if (deal->pending) {
            deal->pending->push_back(std::move(res));
          } else {
            after(res);
          }
        },
        [this, deal](auto &cid) {
          if (auto _data{ipfs_->get(cid)}) {
            if (auto _done{
                    deal->verifier.verifyNextBlock(cid, _data.value())}) {
              deal->state.block(_data.value().size());
              if (_done.value()) {
                deal->all_blocks = true;
                deal->state.last();
              }
              return;
            } else {
              failDeal(deal, _done.error());
            }
          } else {
            failDeal(deal, _data.error());
          }
        });
    return outcome::success();
  }

  void RetrievalClientImpl::processPaymentRequest(
      const std::shared_ptr<DealState> &deal_state,
      const TokenAmount &payment_requested) {
    // check that requested is not exceed total funds for deal
    if (deal_state->state.paid + payment_requested > deal_state->total_funds) {
      failDeal(deal_state, RetrievalClientError::kRequestedTooMuch);
      return;
    }
    // not enough bytes received between payment request
    if (payment_requested != deal_state->state.owed) {
      failDeal(deal_state,
               RetrievalClientError::kBadPaymentRequestBytesNotReceived);
      return;
    }

    auto maybe_voucher =
        api_->PaychVoucherCreate(deal_state->payment_channel_address,
                                 deal_state->state.paid + payment_requested,
                                 deal_state->lane_id);
    if (maybe_voucher.has_error()) {
      failDeal(deal_state, maybe_voucher.error());
      return;
    }
    datatransfer_->pullOut(
        deal_state->pdtid,
        DealPayment::Named::type,
        codec::cbor::encode(
            DealPayment::Named{{deal_state->proposal.deal_id,
                                deal_state->payment_channel_address,
                                maybe_voucher.value()}})
            .value());
    deal_state->state.pay(payment_requested);
  }

  void RetrievalClientImpl::failDeal(
      const std::shared_ptr<DealState> &deal_state,
      const std::error_code &error) {
    deal_state->handler(error);
    datatransfer_->pulling_out.erase(deal_state->pdtid);
  }

}  // namespace fc::markets::retrieval::client
