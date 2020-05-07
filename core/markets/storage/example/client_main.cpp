/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <libp2p/common/literals.hpp>
#include <libp2p/injector/host_injector.hpp>
#include "api/api.hpp"
#include "common/libp2p/cbor_stream.hpp"
#include "crypto/bls/bls_types.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"
#include "host/context/impl/host_context_impl.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/client/client_impl.hpp"
#include "markets/storage/network/libp2p_storage_market_network.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"

namespace fc::markets::storage::example {

  using api::Api;
  using client::ClientImpl;
  using crypto::bls::BlsProvider;
  using crypto::bls::BlsProviderImpl;
  using crypto::secp256k1::Secp256k1ProviderDefault;
  using crypto::secp256k1::Secp256k1Sha256ProviderImpl;
  using fc::storage::ipfs::InMemoryDatastore;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::keystore::InMemoryKeyStore;
  using fc::storage::keystore::KeyStore;
  using libp2p::peer::PeerInfo;
  using pieceio::PieceIO;
  using pieceio::PieceIOImpl;
  using primitives::ChainEpoch;
  using primitives::tipset::Tipset;
  using vm::actor::builtin::miner::MinerInfo;
  using libp2p::common::operator""_unhex;
  using HostContext = fc::host::HostContextImpl;
  using BlsKeyPair = fc::crypto::bls::KeyPair;

  PeerInfo getPeerInfo(std::string conn_string) {
    auto server_ma_res =
        libp2p::multi::Multiaddress::create(conn_string);  // NOLINT
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

  std::shared_ptr<Api> makeApi(const Address &bls_address) {
    std::shared_ptr<Api> api = std::make_shared<Api>();
    api->ChainHead = {[]() {
      ChainEpoch epoch = 100;
      Tipset chain_head;
      chain_head.height = epoch;
      return chain_head;
    }};

    Address worker_address = Address::makeFromId(22);
    api->StateMinerInfo = {
        [worker_address](auto &address,
                         auto &tipset_key) -> outcome::result<MinerInfo> {
          return MinerInfo{.owner = {},
                           .worker = worker_address,
                           .pending_worker_key = boost::none,
                           .peer_id = {},
                           .sector_size = {}};
        }};

    api->StateAccountKey = {
        [worker_address, bls_address](
            auto &address, auto &tipset_key) -> outcome::result<Address> {
          if (address == worker_address) return bls_address;
          throw "Wrong address parameter";
        }};

    return api;
  }

  int main() {
    spdlog::set_level(spdlog::level::debug);

    PeerInfo provider_peer_info = getPeerInfo(
        "/ip4/127.0.0.1/tcp/40010/ipfs/"
        "12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV");

    auto injector = libp2p::injector::makeHostInjector(
        libp2p::injector::useSecurityAdaptors<libp2p::security::Plaintext>());

    auto client_host = injector.create<std::shared_ptr<libp2p::Host>>();
    auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();

    std::shared_ptr<BlsProvider> bls_provider_ =
        std::make_shared<BlsProviderImpl>();
    std::shared_ptr<Secp256k1ProviderDefault> secp256k1_provider_ =
        std::make_shared<Secp256k1Sha256ProviderImpl>();
    std::shared_ptr<KeyStore> keystore =
        std::make_shared<InMemoryKeyStore>(bls_provider_, secp256k1_provider_);

    crypto::bls::PrivateKey bls_private_key{};
    auto priv_key_bytes =
        "8e8c5263df0022d8e29cab943d57d851722c38ee1dbe7f8c29c0498156496f29"_unhex;
    std::copy_n(priv_key_bytes.begin(),
                bls_private_key.size(),
                bls_private_key.begin());
    auto bls_public_key =
        bls_provider_->derivePublicKey(bls_private_key).value();
    Address bls_address = Address::makeBls(bls_public_key);
    std::shared_ptr<Api> api = makeApi(bls_address);

    std::shared_ptr<IpfsDatastore> datastore =
        std::make_shared<InMemoryDatastore>();
    std::shared_ptr<PieceIO> piece_io =
        std::make_shared<PieceIOImpl>(datastore);

    auto fsm_context = std::make_shared<HostContext>();

    std::shared_ptr<ClientImpl> client = std::make_shared<ClientImpl>(
        client_host, context, api, keystore, piece_io, fsm_context);

    StorageProviderInfo info{.address = Address::makeFromId(1),
                             .owner = {},
                             .worker = {},
                             .sector_size = {},
                             .peer_info = provider_peer_info};

    client->getAsk(info, [](outcome::result<SignedStorageAsk> ask_res) {
      if (ask_res.has_error()) {
        std::cout << "response error " << ask_res.error().message()
                  << std::endl;
      } else {
        std::cout << "response read price " << ask_res.value().ask.price
                  << std::endl;
        std::cout << "response read expiry " << ask_res.value().ask.expiry
                  << std::endl;
      }
    });

    context->run_for(std::chrono::seconds(10));

    return 0;
  }

}  // namespace fc::markets::storage::example

int main() {
  return fc::markets::storage::example::main();
}
