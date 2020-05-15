/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/example/storage_market_example.hpp"

#include <iostream>
#include <libp2p/common/literals.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/security/plaintext.hpp>
#include "adt/channel.hpp"
#include "api/miner_api.hpp"
#include "common/buffer.hpp"
#include "common/span.hpp"
#include "common/todo_error.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "markets/storage/example/client_example.hpp"
#include "markets/storage/example/provider_example.hpp"
#include "markets/storage/example/resources.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"

namespace fc::markets::storage::example {

  using adt::Channel;
  using api::MinerApi;
  using api::MsgWait;
  using api::Wait;
  using common::Buffer;
  using crypto::bls::BlsProvider;
  using crypto::bls::BlsProviderImpl;
  using crypto::secp256k1::Secp256k1ProviderDefault;
  using crypto::secp256k1::Secp256k1Sha256ProviderImpl;
  using fc::storage::InMemoryStorage;
  using fc::storage::ipfs::InMemoryDatastore;
  using fc::storage::ipfs::IpfsDatastore;
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  using pieceio::PieceIO;
  using pieceio::PieceIOImpl;
  using primitives::DealId;
  using primitives::sector::RegisteredProof;
  using provider::Datastore;
  using vm::VMExitCode;
  using vm::actor::builtin::market::PublishStorageDeals;
  using vm::runtime::MessageReceipt;
  using libp2p::common::operator""_unhex;
  using fc::storage::piece::PieceInfo;
  using primitives::GasAmount;
  using primitives::tipset::Tipset;
  using vm::actor::builtin::miner::MinerInfo;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using BlsKeyPair = fc::crypto::bls::KeyPair;

  static const RegisteredProof kRegisteredProof{
      RegisteredProof::StackedDRG32GiBSeal};

  /**
   * Payload CID from Go
   */
  static const std::string kPayloadCid =
      "QmXFz92Uc9gCyAVGKkCzD84HEiR9fmrFzPSrvUypaN2Yzx";

  static const Address kMinerActorAddress = Address::makeFromId(22);
  static const Address kClientAddress = Address::makeFromId(333);

  /**
   * Reads car file into Buffer
   * @param path to file
   * @return car file content as Buffer
   */
  outcome::result<Buffer> readFile(const std::string &path) {
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    if (!file.good()) {
      std::cerr << "Cannot open file: " << path;
      return TodoError::ERROR;
    }
    Buffer buffer;
    buffer.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(fc::common::span::string(buffer).data(), buffer.size());
    return buffer;
  }

  outcome::result<DataRef> makeDataRef(std::shared_ptr<PieceIO> &piece_io,
                                       const Buffer &data) {
    OUTCOME_TRY(root_cid, CID::fromString(kPayloadCid));
    OUTCOME_TRY(piece_commitment,
                piece_io->generatePieceCommitment(kRegisteredProof, data));
    return DataRef{.transfer_type = kTransferTypeManual,
                   .root = root_cid,
                   .piece_cid = piece_commitment.first,
                   .piece_size = piece_commitment.second};
  }

  /**
   * Makes libp2p::PeerInfo from connection string
   * @param conn_string
   * @return libp2p::PeerInfo
   */
  PeerInfo getPeerInfo(const std::string &conn_string) {
    auto server_ma_res = libp2p::multi::Multiaddress::create(conn_string);
    if (!server_ma_res) {
      std::cerr << "unable to create server multiaddress: "
                << server_ma_res.error().message() << std::endl;
      std::exit(EXIT_FAILURE);
    }
    auto server_ma = std::move(server_ma_res.value());

    auto server_peer_id_str = server_ma.getPeerId();
    if (!server_peer_id_str) {
      std::cerr << "unable to get peer id" << std::endl;
      std::exit(EXIT_FAILURE);
    }

    auto server_peer_id_res =
        libp2p::peer::PeerId::fromBase58(*server_peer_id_str);
    if (!server_peer_id_res) {
      std::cerr << "Unable to decode peer id from base 58: "
                << server_peer_id_res.error().message() << std::endl;
      std::exit(EXIT_FAILURE);
    }

    auto server_peer_id = std::move(server_peer_id_res.value());

    return PeerInfo{server_peer_id, {server_ma}};
  }

  StorageProviderInfo makeStorageProviderInfo() {
    PeerInfo provider_peer_info = getPeerInfo(
        kProviderAddress
        + "/ipfs/12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV");
    return StorageProviderInfo{
        .address = kMinerActorAddress,
        .owner = {},
        .worker = {},
        .sector_size = SectorSize{1000000},  // large enough
        .peer_info = provider_peer_info};
  }

  std::shared_ptr<Api> makeApi(
      const BlsKeyPair &miner_worker_keypair,
      const Address &provider_actor_address,
      const Address &client_id_address,
      const BlsKeyPair &client_keypair,
      const std::shared_ptr<BlsProvider> &bls_provider) {
    Address miner_worker_address =
        Address::makeBls(miner_worker_keypair.public_key);
    Address client_bls_address = Address::makeBls(client_keypair.public_key);
    ChainEpoch epoch = 100;
    Tipset chain_head;
    chain_head.height = epoch;

    std::shared_ptr<Api> api = std::make_shared<Api>();
    api->ChainHead = {[chain_head]() { return chain_head; }};

    api->StateMinerInfo = {
        [](auto &address, auto &tipset_key) -> outcome::result<MinerInfo> {
          return MinerInfo{.owner = {},
                           .worker = kMinerActorAddress,
                           .pending_worker_key = boost::none,
                           .peer_id = {},
                           .sector_size = {}};
        }};

    api->MarketEnsureAvailable = {
        [](auto, auto, auto, auto) -> boost::optional<CID> {
          // funds ensured
          return boost::none;
        }};

    api->StateAccountKey = {
        [provider_actor_address,
            miner_worker_address,
         client_id_address,
         client_bls_address](auto &address,
                             auto &tipset_key) -> outcome::result<Address> {
          if (address == provider_actor_address) return miner_worker_address;
          if (address == client_id_address) return client_bls_address;
          throw "StateAccountKey: Wrong address parameter";
        }};

    api->MpoolPushMessage = {
        [bls_provider, miner_worker_keypair](
            auto &unsigned_message) -> outcome::result<SignedMessage> {
          if (unsigned_message.from == kMinerActorAddress) {
            OUTCOME_TRY(encoded_message, codec::cbor::encode(unsigned_message));
            OUTCOME_TRY(signature,
                        bls_provider->sign(encoded_message,
                                           miner_worker_keypair.private_key));
            return SignedMessage{.message = unsigned_message,
                                 .signature = signature};
          };
          throw "MpoolPushMessage: Wrong from address parameter";
        }};

    api->StateWaitMsg = {
        [chain_head](auto &message_cid) -> outcome::result<Wait<MsgWait>> {
          std::cout << "StateWaitMsg called for message cid "
                    << message_cid.toString().value() << std::endl;
          PublishStorageDeals::Result publish_deal_result{};
          publish_deal_result.deals.emplace_back(1);
          auto publish_deal_result_encoded =
              codec::cbor::encode(publish_deal_result).value();

          MsgWait message_result{
              .receipt =
                  MessageReceipt{.exit_code = VMExitCode::Ok,
                                 .return_value = publish_deal_result_encoded,
                                 .gas_used = GasAmount{0}},
              .tipset = chain_head};
          auto channel = std::make_shared<Channel<Wait<MsgWait>::Result>>();
          channel->write(message_result);
          channel->closeWrite();
          Wait<MsgWait> wait_msg{channel};
          return wait_msg;
        }};

    return api;
  }

  std::shared_ptr<MinerApi> makeMinerApi() {
    std::shared_ptr<MinerApi> miner_api = std::make_shared<MinerApi>();

    miner_api->LocatePieceForDealWithinSector = {
        [](auto &deal_id, auto &tipset_key) -> outcome::result<PieceInfo> {
          return PieceInfo{};
        }};

    return miner_api;
  }

  outcome::result<void> makeExample() {
    spdlog::set_level(spdlog::level::debug);

    std::shared_ptr<BlsProvider> bls_provider =
        std::make_shared<BlsProviderImpl>();
    std::shared_ptr<Secp256k1ProviderDefault> secp256k1_provider =
        std::make_shared<Secp256k1Sha256ProviderImpl>();
    std::shared_ptr<Datastore> datastore = std::make_shared<InMemoryStorage>();
    std::shared_ptr<IpfsDatastore> ipfs_datastore =
        std::make_shared<InMemoryDatastore>();
    std::shared_ptr<PieceIO> piece_io =
        std::make_shared<PieceIOImpl>(ipfs_datastore);

    OUTCOME_TRY(miner_worker_keypair, bls_provider->generateKeyPair());
    OUTCOME_TRY(client_keypair, bls_provider->generateKeyPair());
    std::shared_ptr<Api> api = makeApi(miner_worker_keypair,
                                       kMinerActorAddress,
                                       kClientAddress,
                                       client_keypair,
                                       bls_provider);
    std::shared_ptr<MinerApi> miner_api = makeMinerApi();

    // Initialize provider

    // resulting PeerId should be
    // 12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV
    KeyPair keypair{PublicKey{{Key::Type::Ed25519,
                               "48453469c62f4885373099421a7365520b5ffb"
                               "0d93726c124166be4b81d852e6"_unhex}},
                    PrivateKey{{Key::Type::Ed25519,
                                "4a9361c525840f7086b893d584ebbe475b4ec"
                                "7069951d2e897e8bceb0a3f35ce"_unhex}}};

    auto injector = libp2p::injector::makeHostInjector(
        libp2p::injector::useKeyPair(keypair),
        libp2p::injector::useSecurityAdaptors<libp2p::security::Plaintext>());
    auto host = injector.create<std::shared_ptr<libp2p::Host>>();

    auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();

    StorageProviderInfo provider_info = makeStorageProviderInfo();
    OUTCOME_TRY(provider,
                makeProvider(provider_info.peer_info.addresses.front(),
                             kRegisteredProof,
                             miner_worker_keypair,
                             bls_provider,
                             secp256k1_provider,
                             datastore,
                             piece_io,
                             host,
                             context,
                             api,
                             miner_api,
                             kMinerActorAddress));

    TokenAmount provider_price = 1334;
    ChainEpoch duration = 2334;
    OUTCOME_TRY(provider->addAsk(provider_price, duration));
    OUTCOME_TRY(provider->start());

    // Initialize client
    OUTCOME_TRY(client,
                makeClient(client_keypair,
                           bls_provider,
                           secp256k1_provider,
                           piece_io,
                           host,
                           context,
                           api));

    // send ask request
    sendGetAsk(client, provider_info);

    // propose storage deal
    OUTCOME_TRY(data, readFile(CAR_FROM_PAYLOAD_FILE));
    OUTCOME_TRY(data_ref, makeDataRef(piece_io, data));
    ChainEpoch start_epoch{10};
    ChainEpoch end_epoch{200};
    TokenAmount client_price{10};
    TokenAmount collateral{10};
    OUTCOME_TRY(proposal_res,
                client->proposeStorageDeal(kClientAddress,
                                           provider_info,
                                           data_ref,
                                           start_epoch,
                                           end_epoch,
                                           client_price,
                                           collateral,
                                           kRegisteredProof));
    auto proposal_cid = proposal_res.proposal_cid;

    context->run_for(std::chrono::seconds(3));
    std::cout << "Import data for deal " << proposal_cid.toString().value()
              << std::endl;
    OUTCOME_TRY(provider->importDataForDeal(proposal_cid, data));
    context->run_for(std::chrono::seconds(5));

    return outcome::success();
  }

}  // namespace fc::markets::storage::example

int main() {
  fc::markets::storage::example::makeExample().value();
}
