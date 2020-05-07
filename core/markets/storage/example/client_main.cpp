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
#include "primitives/sector/sector.hpp"
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
  using libp2p::multi::HashType;
  using libp2p::multi::MulticodecType;
  using libp2p::multi::Multihash;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::RegisteredProof;
  using BlsKeyPair = fc::crypto::bls::KeyPair;
  using HostContext = fc::host::HostContextImpl;

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

  StorageProviderInfo makeStorageProviderInfo() {
    PeerInfo provider_peer_info = getPeerInfo(
        "/ip4/127.0.0.1/tcp/40010/ipfs/"
        "12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV");
    return StorageProviderInfo{
        .address = Address::makeFromId(1),
        .owner = {},
        .worker = {},
        .sector_size = SectorSize{1000000},  // large enough
        .peer_info = provider_peer_info};
  }

  std::shared_ptr<ClientImpl> makeClient(
      const std::shared_ptr<libp2p::Host> &client_host,
      const std::shared_ptr<boost::asio::io_context> &context) {
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

    keystore->put(bls_address, bls_private_key);

    std::shared_ptr<IpfsDatastore> datastore =
        std::make_shared<InMemoryDatastore>();
    std::shared_ptr<PieceIO> piece_io =
        std::make_shared<PieceIOImpl>(datastore);

    auto fsm_context = std::make_shared<HostContext>();

    return std::make_shared<ClientImpl>(
        client_host, context, api, keystore, piece_io, fsm_context);
  }

  void sendGetAsk(const StorageProviderInfo &info,
                  const std::shared_ptr<ClientImpl> &client) {
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
  }

  void sendProposeDeal(const StorageProviderInfo &info,
                       const std::shared_ptr<ClientImpl> &client) {
    CID cid{CID::Version::V1,
            MulticodecType::SHA2_256,
            Multihash::create(HashType::sha256,
                              "0123456789ABCDEF0123456789ABCDEF"_unhex)
                .value()};

    Address address = Address::makeFromId(22);

    DataRef data_ref{.transfer_type = kTransferTypeManual,
                     .root = cid,
                     .piece_cid = cid,
                     .piece_size = UnpaddedPieceSize{100500}};
    ChainEpoch start_epoch{200};
    ChainEpoch end_epoch{33333};
    TokenAmount price{1334};
    TokenAmount collateral{3556};
    RegisteredProof registered_proof{RegisteredProof::StackedDRG32GiBSeal};

    auto res = client->proposeStorageDeal(
        address,
        info,
        data_ref,
        start_epoch,
        end_epoch,
        price,
        collateral,
        registered_proof,
        [](outcome::result<void> proposal_res) {
          if (proposal_res.has_error()) {
            std::cout << "response error " << proposal_res.error().message()
                      << std::endl;
          } else {
            std::cout << "propose success" << std::endl;
          }
        });

    if (res.has_error()) {
      std::cerr << res.error().message();
    }
  }

  int main() {
    spdlog::set_level(spdlog::level::debug);

    auto injector = libp2p::injector::makeHostInjector(
        libp2p::injector::useSecurityAdaptors<libp2p::security::Plaintext>());

    auto client_host = injector.create<std::shared_ptr<libp2p::Host>>();
    auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();

    StorageProviderInfo info = makeStorageProviderInfo();
    std::shared_ptr<ClientImpl> client = makeClient(client_host, context);

    // send ask request
    // sendGetAsk(info, client);

    // propose storage deal
    sendProposeDeal(info, client);

    client->run();

    context->run_for(std::chrono::seconds(10));

    return 0;
  }

}  // namespace fc::markets::storage::example

int main() {
  return fc::markets::storage::example::main();
}
