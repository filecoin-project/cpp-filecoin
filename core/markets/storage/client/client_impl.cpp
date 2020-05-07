/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/client/client_impl.hpp"

#include <libp2p/peer/peer_id.hpp>
#include "codec/cbor/cbor.hpp"
#include "crypto/hasher/hasher.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/client/client_fsm_transitions.hpp"
#include "storage/ipld/impl/ipld_node_impl.hpp"
#include "vm/message/message.hpp"
#include "vm/message/message_util.hpp"

namespace fc::markets::storage::client {

  using crypto::Hasher;
  using libp2p::peer::PeerId;
  using primitives::BigInt;
  using primitives::GasAmount;
  using vm::VMExitCode;
  using vm::actor::kStorageMarketAddress;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  // from lotus
  // https://github.com/filecoin-project/lotus/blob/7e0be91cfd44c1664ac18f81080544b1341872f1/markets/storageadapter/client.go#L122
  const BigInt kGasPrice{0};
  const GasAmount kGasLimit{1000000};

  ClientImpl::ClientImpl(
      std::shared_ptr<Host> host,
      std::shared_ptr<boost::asio::io_context> context,
      std::shared_ptr<Api> api,
      std::shared_ptr<KeyStore> keystore,
      std::shared_ptr<PieceIO> piece_io,
      const std::shared_ptr<fc::host::HostContext> &fsm_constext)
      : host_{std::move(host)},
        context_{std::move(context)},
        api_{std::move(api)},
        keystore_{std::move(keystore)},
        piece_io_{std::move(piece_io)},
        network_{std::make_shared<Libp2pStorageMarketNetwork>(host_)},
        fsm_{std::make_shared<ClientFSM>(client_transitions, fsm_constext)} {}

  void ClientImpl::run() {}

  void ClientImpl::stop() {}

  outcome::result<std::vector<StorageProviderInfo>> ClientImpl::listProviders()
      const {
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

  outcome::result<std::vector<StorageDeal>> ClientImpl::listDeals(
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

  outcome::result<std::vector<ClientDeal>> ClientImpl::listLocalDeals() const {
    std::vector<ClientDeal> res;
    for (auto deal : local_deals_) {
      res.push_back(ClientDeal{*deal.second});
    }
    return res;
  }

  outcome::result<ClientDeal> ClientImpl::getLocalDeal(const CID &cid) const {
    auto it = local_deals_.find(cid);
    if (it == local_deals_.end()) {
      return StorageMarketClientError::LOCAL_DEAL_NOT_FOUND;
    }
    return *it->second;
  }

  void ClientImpl::getAsk(const StorageProviderInfo &info,
                          const SignedAskHandler &signed_ask_handler) const {
    network_->newAskStream(
        info.peer_info,
        [self{shared_from_this()}, info, signed_ask_handler](
            auto &&stream_res) {
          if (stream_res.has_error()) {
            self->logger_->error("Cannot open stream to "
                                 + info.peer_info.id.toBase58() + ": "
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
                                              "Cannot send request ",
                                              stream,
                                              signed_ask_handler))
                            return;
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

  outcome::result<ProposeStorageDealResult> ClientImpl::proposeStorageDeal(
      const Address &address,
      const StorageProviderInfo &provider_info,
      const DataRef &data_ref,
      const ChainEpoch &start_epoch,
      const ChainEpoch &end_epoch,
      const TokenAmount &price,
      const TokenAmount &collateral,
      const RegisteredProof &registered_proof,
      const ProposalHandler &proposal_handler) {
    OUTCOME_TRY(comm_p_res, calculateCommP(registered_proof, data_ref));
    CID comm_p = comm_p_res.first;
    UnpaddedPieceSize piece_size = comm_p_res.second;
    if (piece_size.padded() > provider_info.sector_size) {
      return StorageMarketClientError::PIECE_SIZE_GREATER_SECTOR_SIZE;
    }

    DealProposal deal_proposal{
        .piece_cid = comm_p,
        .piece_size = piece_size.padded(),
        .client = address,
        .provider = provider_info.address,
        .start_epoch = start_epoch,
        .end_epoch = end_epoch,
        .storage_price_per_epoch = price,
        .provider_collateral = static_cast<uint64_t>(piece_size),
        .client_collateral = 0};
    OUTCOME_TRY(signed_proposal, signProposal(address, deal_proposal));
    OUTCOME_TRY(proposal_cid, getProposalCid(signed_proposal));

    auto client_deal = std::make_shared<ClientDeal>(
        ClientDeal{.client_deal_proposal = signed_proposal,
                   .proposal_cid = proposal_cid,
                   .add_funds_cid = {},
                   .state = StorageDealStatus::STORAGE_DEAL_UNKNOWN,
                   .miner = provider_info.peer_info,
                   .miner_worker = provider_info.worker,
                   .deal_id = {},
                   .data_ref = data_ref,
                   .message = {},
                   .publish_message = {}});
    local_deals_[client_deal->proposal_cid] = client_deal;

    // TODO handler
    network_->newDealStream(
        provider_info.peer_info,
        [self{shared_from_this()},
         provider_info,
         deal_proposal,
         proposal_handler](
            outcome::result<std::shared_ptr<CborStream>> stream_res) {
          if (stream_res.has_error()) {
            self->logger_->error("Cannot open stream to "
                                 + provider_info.peer_info.id.toBase58() + ": "
                                 + stream_res.error().message());
            return;
          }

          // TODO ensure funds, etc

          // OUTCOME_TRY(fsm_->begin(client_deal,
          // StorageDealStatus::STORAGE_DEAL_UNKNOWN));

          // TODO connection manager - add stream
          // OUTCOME_TRY(fsm_->send(client_deal, ClientEvent::ClientEventOpen));

          stream_res.value()->write(
              deal_proposal,
              [self, proposal_handler](outcome::result<size_t> written) {
                proposal_handler(outcome::success());
              });
        });

    // TODO discovery - add peer (address: dealProposal.Provider, ID:
    // deal.Miner)

    return ProposeStorageDealResult{client_deal->proposal_cid};
  }

  outcome::result<StorageParticipantBalance> ClientImpl::getPaymentEscrow(
      const Address &address) const {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    return api_->StateMarketBalance(address, tipset_key);
  }

  outcome::result<void> ClientImpl::addPaymentEscrow(
      const Address &address, const TokenAmount &amount) {
    UnsignedMessage unsigned_message{
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

  outcome::result<SignedStorageAsk> ClientImpl::validateAskResponse(
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
      return StorageMarketClientError::SIGNATURE_INVALID;
    }
    return response.value().ask;
  }

  outcome::result<std::pair<CID, UnpaddedPieceSize>> ClientImpl::calculateCommP(
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

  outcome::result<ClientDealProposal> ClientImpl::signProposal(
      const Address &address, const DealProposal &proposal) const {
    OUTCOME_TRY(chain_head, api_->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());
    OUTCOME_TRY(key_address, api_->StateAccountKey(address, tipset_key));
    OUTCOME_TRY(proposal_bytes, codec::cbor::encode(proposal));
    OUTCOME_TRY(signature, keystore_->sign(key_address, proposal_bytes));
    return ClientDealProposal{.proposal = proposal,
                              .client_signature = signature};
  }

  outcome::result<CID> ClientImpl::getProposalCid(
      const ClientDealProposal &signed_proposal) const {
    OUTCOME_TRY(bytes, codec::cbor::encode(signed_proposal));
    auto hash = Hasher::sha2_256(bytes);
    return CID(CID::Version::V1, CID::Multicodec::DAG_CBOR, hash);
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
