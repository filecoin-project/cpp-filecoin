/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_CORE_MARKETS_STORAGE_STORAGE_MARKET_FIXTURE_HPP
#define CPP_FILECOIN_TEST_CORE_MARKETS_STORAGE_STORAGE_MARKET_FIXTURE_HPP

#include <gtest/gtest.h>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/security/plaintext.hpp>
#include "api/api.hpp"
#include "api/miner_api.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/client/client_impl.hpp"
#include "markets/storage/provider/provider.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "testutil/literals.hpp"

namespace fc::markets::storage::test {
  using adt::Channel;
  using api::Api;
  using api::MinerApi;
  using api::MsgWait;
  using api::Wait;
  using client::Client;
  using client::ClientImpl;
  using crypto::bls::BlsProvider;
  using crypto::bls::BlsProviderImpl;
  using crypto::secp256k1::Secp256k1ProviderDefault;
  using crypto::secp256k1::Secp256k1Sha256ProviderImpl;
  using fc::storage::InMemoryStorage;
  using fc::storage::ipfs::InMemoryDatastore;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::keystore::InMemoryKeyStore;
  using fc::storage::keystore::KeyStore;
  using fc::storage::piece::PieceInfo;
  using libp2p::Host;
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  using libp2p::multi::Multiaddress;
  using pieceio::PieceIO;
  using pieceio::PieceIOImpl;
  using primitives::GasAmount;
  using primitives::sector::RegisteredProof;
  using primitives::tipset::Tipset;
  using provider::Datastore;
  using provider::StorageProvider;
  using provider::StorageProviderImpl;
  using vm::VMExitCode;
  using vm::actor::builtin::market::PublishStorageDeals;
  using vm::actor::builtin::miner::MinerInfo;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using vm::runtime::MessageReceipt;
  using BlsKeyPair = fc::crypto::bls::KeyPair;

  class StorageMarketTest : public ::testing::Test {
   public:
    void SetUp() override {
      std::string address_string =
          "/ip4/127.0.0.1/tcp/40010/ipfs/"
          "12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV";

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
      auto provider_multiaddress = Multiaddress::create(address_string).value();
      OUTCOME_EXCEPT(host->listen(provider_multiaddress));

      context_ = injector.create<std::shared_ptr<boost::asio::io_context>>();

      std::shared_ptr<BlsProvider> bls_provider =
          std::make_shared<BlsProviderImpl>();
      std::shared_ptr<Secp256k1ProviderDefault> secp256k1_provider =
          std::make_shared<Secp256k1Sha256ProviderImpl>();
      std::shared_ptr<Datastore> datastore =
          std::make_shared<InMemoryStorage>();
      std::shared_ptr<IpfsDatastore> ipfs_datastore =
          std::make_shared<InMemoryDatastore>();
      std::shared_ptr<PieceIO> piece_io =
          std::make_shared<PieceIOImpl>(ipfs_datastore);

      auto miner_actor_address = Address::makeFromId(100);
      auto client_id_address = Address::makeFromId(102);
      OUTCOME_EXCEPT(miner_worker_keypair, bls_provider->generateKeyPair());
      OUTCOME_EXCEPT(client_keypair, bls_provider->generateKeyPair());

      RegisteredProof registered_proof{RegisteredProof::StackedDRG32GiBSeal};

      auto node_api = makeNodeApi(miner_actor_address,
                                  miner_worker_keypair,
                                  client_id_address,
                                  client_keypair,
                                  bls_provider);
      auto miner_api = makeMinerApi();

      provider = makeProvider(provider_multiaddress,
                              registered_proof,
                              miner_worker_keypair,
                              bls_provider,
                              secp256k1_provider,
                              datastore,
                              piece_io,
                              host,
                              context_,
                              node_api,
                              miner_api,
                              miner_actor_address);
      OUTCOME_EXCEPT(provider->start());

      client = makeClient(client_keypair,
                          bls_provider,
                          secp256k1_provider,
                          piece_io,
                          host,
                          context_,
                          node_api);
      storage_provider_info = makeStorageProviderInfo(
          miner_actor_address, host, provider_multiaddress);

      logger->debug(
          "Provider info "
          + peerInfoToPrettyString(storage_provider_info.get()->peer_info));

      std::thread([this]() { context_->run(); }).detach();
    }

    void TearDown() override {
      context_->stop();
    }

   protected:
    /**
     * Makes node API
     * @param miner_actor_address
     * @param miner_worker_keypair
     * @param provider_keypair
     * @param client_id_address
     * @param client_keypair
     * @param bls_provider
     * @return
     */
    std::shared_ptr<Api> makeNodeApi(
        const Address &miner_actor_address,
        const BlsKeyPair &miner_worker_keypair,
        const Address &client_id_address,
        const BlsKeyPair &client_keypair,
        const std::shared_ptr<BlsProvider> &bls_provider) {
      Address miner_worker_address =
          Address::makeBls(miner_worker_keypair.public_key);
      Address client_bls_address = Address::makeBls(client_keypair.public_key);
      ChainEpoch epoch = 100;
      chain_head.height = epoch;

      std::shared_ptr<Api> api = std::make_shared<Api>();
      api->ChainHead = {[this]() { return chain_head; }};

      api->StateMinerInfo = {
          [miner_actor_address](
              auto &address, auto &tipset_key) -> outcome::result<MinerInfo> {
            return MinerInfo{.owner = {},
                             .worker = miner_actor_address,
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
          [this,
           miner_worker_address,
           miner_actor_address,
           client_id_address,
           client_bls_address](auto &address,
                               auto &tipset_key) -> outcome::result<Address> {
            if (address == miner_actor_address) return miner_worker_address;
            if (address == client_id_address) return client_bls_address;
            logger->error("StateAccountKey: Wrong address parameter");
            throw "StateAccountKey: Wrong address parameter";
          }};

      api->MpoolPushMessage = {
          [bls_provider, miner_worker_keypair, miner_actor_address](
              auto &unsigned_message) -> outcome::result<SignedMessage> {
            if (unsigned_message.from == miner_actor_address) {
              OUTCOME_TRY(encoded_message,
                          codec::cbor::encode(unsigned_message));
              OUTCOME_TRY(signature,
                          bls_provider->sign(encoded_message,
                                             miner_worker_keypair.private_key));
              return SignedMessage{.message = unsigned_message,
                                   .signature = signature};
            };
            throw "MpoolPushMessage: Wrong from address parameter";
          }};

      api->StateWaitMsg = {
          [this](auto &message_cid) -> outcome::result<Wait<MsgWait>> {
            logger->debug("StateWaitMsg called for message cid "
                          + message_cid.toString().value());
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

    /**
     * Makes miner API
     * @return
     */
    std::shared_ptr<MinerApi> makeMinerApi() {
      std::shared_ptr<MinerApi> miner_api = std::make_shared<MinerApi>();

      miner_api->LocatePieceForDealWithinSector = {
          [](auto &deal_id, auto &tipset_key) -> outcome::result<PieceInfo> {
            return PieceInfo{};
          }};

      return miner_api;
    }

    std::shared_ptr<StorageProviderImpl> makeProvider(
        const Multiaddress &provider_multiaddress,
        const RegisteredProof &registered_proof,
        const BlsKeyPair &miner_worker_keypair,
        const std::shared_ptr<BlsProvider> &bls_provider,
        const std::shared_ptr<Secp256k1ProviderDefault> &secp256k1_provider,
        const std::shared_ptr<Datastore> &datastore,
        const std::shared_ptr<PieceIO> piece_io,
        const std::shared_ptr<libp2p::Host> &provider_host,
        const std::shared_ptr<boost::asio::io_context> &context,
        const std::shared_ptr<Api> &api,
        const std::shared_ptr<MinerApi> &miner_api,
        const Address &miner_actor_address) {
      std::shared_ptr<KeyStore> keystore =
          std::make_shared<InMemoryKeyStore>(bls_provider, secp256k1_provider);

      Address miner_worker_address =
          Address::makeBls(miner_worker_keypair.public_key);
      OUTCOME_EXCEPT(keystore->put(miner_worker_address,
                                   miner_worker_keypair.private_key));

      std::shared_ptr<StorageProviderImpl> new_provider =
          std::make_shared<StorageProviderImpl>(registered_proof,
                                                provider_host,
                                                context,
                                                keystore,
                                                datastore,
                                                api,
                                                miner_api,
                                                miner_actor_address,
                                                piece_io);
      new_provider->init();
      return new_provider;
    }

    std::shared_ptr<StorageProviderInfo> makeStorageProviderInfo(
        const Address &miner_actor_address,
        const std::shared_ptr<Host> &provider_host,
        const Multiaddress &multi_address) {
      return std::make_shared<StorageProviderInfo>(StorageProviderInfo{
          .address = miner_actor_address,
          .owner = {},
          .worker = {},
          .sector_size = SectorSize{1000000},
          .peer_info = PeerInfo{provider_host->getId(), {multi_address}}});
    }

    std::shared_ptr<Client> makeClient(
        const BlsKeyPair &client_keypair,
        const std::shared_ptr<BlsProvider> &bls_provider,
        const std::shared_ptr<Secp256k1ProviderDefault> &secp256k1_provider,
        const std::shared_ptr<PieceIO> &piece_io,
        const std::shared_ptr<libp2p::Host> &client_host,
        const std::shared_ptr<boost::asio::io_context> &context,
        const std::shared_ptr<Api> &api) {
      std::shared_ptr<KeyStore> keystore =
          std::make_shared<InMemoryKeyStore>(bls_provider, secp256k1_provider);

      Address bls_address = Address::makeBls(client_keypair.public_key);
      OUTCOME_EXCEPT(keystore->put(bls_address, client_keypair.private_key));

      auto new_client = std::make_shared<ClientImpl>(
          client_host, context, api, keystore, piece_io);
      new_client->init();
      return new_client;
    }

    common::Logger logger = common::createLogger("StorageMarketProvider");

    Tipset chain_head;
    std::shared_ptr<Client> client;
    std::shared_ptr<StorageProvider> provider;
    std::shared_ptr<StorageProviderInfo> storage_provider_info;

   protected:
    std::shared_ptr<boost::asio::io_context> context_;
  };

}  // namespace fc::markets::storage::test

#endif  // CPP_FILECOIN_TEST_CORE_MARKETS_STORAGE_STORAGE_MARKET_FIXTURE_HPP
