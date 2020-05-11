/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <libp2p/common/literals.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/security/plaintext.hpp>
#include "api/api.hpp"
#include "crypto/bls/bls_types.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/example/storage_market_example.hpp"
#include "markets/storage/network/libp2p_storage_market_network.hpp"
#include "markets/storage/provider/provider.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"

namespace fc::markets::storage::example {

  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  using libp2p::peer::PeerInfo;
  using libp2p::common::operator""_unhex;
  using api::Api;
  using common::Buffer;
  using fc::crypto::bls::BlsProvider;
  using fc::crypto::bls::BlsProviderImpl;
  using fc::crypto::secp256k1::Secp256k1ProviderDefault;
  using fc::crypto::secp256k1::Secp256k1Sha256ProviderImpl;
  using fc::storage::InMemoryStorage;
  using fc::storage::ipfs::InMemoryDatastore;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::keystore::InMemoryKeyStore;
  using fc::storage::keystore::KeyStore;
  using pieceio::PieceIO;
  using pieceio::PieceIOImpl;
  using primitives::ChainEpoch;
  using primitives::address::Address;
  using primitives::sector::RegisteredProof;
  using primitives::tipset::Tipset;
  using provider::Datastore;
  using provider::StorageProvider;
  using provider::StorageProviderImpl;
  using BlsKeyPair = fc::crypto::bls::KeyPair;

  std::shared_ptr<Api> makeApi(const Address &bls_address) {
    std::shared_ptr<Api> api = std::make_shared<Api>();
    api->ChainHead = {[]() {
      ChainEpoch epoch = 100;
      Tipset chain_head;
      chain_head.height = epoch;
      return chain_head;
    }};

    api->StateAccountKey = {
        [bls_address](auto &address,
                      auto &tipset_key) -> outcome::result<Address> {
          return bls_address;
        }};

    return api;
  }

  std::shared_ptr<StorageProviderImpl> makeProvider(
      std::shared_ptr<libp2p::Host> provider_host,
      const std::shared_ptr<boost::asio::io_context> &context) {
    RegisteredProof registered_proof{RegisteredProof::StackedDRG32GiBSeal};

    auto ma = libp2p::multi::Multiaddress::create(kProviderAddress).value();
    provider_host->listen(ma);

    std::shared_ptr<BlsProvider> bls_provider_ =
        std::make_shared<BlsProviderImpl>();
    std::shared_ptr<Secp256k1ProviderDefault> secp256k1_provider_ =
        std::make_shared<Secp256k1Sha256ProviderImpl>();
    std::shared_ptr<KeyStore> keystore =
        std::make_shared<InMemoryKeyStore>(bls_provider_, secp256k1_provider_);

    std::shared_ptr<Datastore> datastore = std::make_shared<InMemoryStorage>();

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

    Address actor_address = Address::makeFromId(1);

    std::shared_ptr<IpfsDatastore> ipfs_datastore =
        std::make_shared<InMemoryDatastore>();
    std::shared_ptr<PieceIO> piece_io =
        std::make_shared<PieceIOImpl>(ipfs_datastore);

    std::shared_ptr<StorageProviderImpl> provider =
        std::make_shared<StorageProviderImpl>(registered_proof,
                                              provider_host,
                                              context,
                                              keystore,
                                              datastore,
                                              api,
                                              actor_address,
                                              piece_io);
    provider->init();
    return provider;
  }

  void importDataForDeal(const std::shared_ptr<StorageProviderImpl> &provider,
                         const std::string &kProposalCid,
                         const Buffer &data) {
    auto proposal_cid = CID::fromString(kProposalCid);
    if (proposal_cid.has_error()) {
      std::cerr << "Cannot make proposal CID" << proposal_cid.error().message()
                << std::endl;
      return;
    }
    auto res = provider->importDataForDeal(proposal_cid.value(), data);
    if (res.has_error()) {
      std::cerr << "Cannot import data for deal" << res.error().message()
                << std::endl;
    }
  }

  int main() {
    spdlog::set_level(spdlog::level::debug);

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
    auto provider_host = injector.create<std::shared_ptr<libp2p::Host>>();

    auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();

    auto provider = makeProvider(provider_host, context);

    TokenAmount price = 1334;
    ChainEpoch duration = 2334;
    provider->addAsk(price, duration);
    provider->start();

    try {
      context->run_for(std::chrono::seconds(20));
      std::cout << "Import data for deal " << kProposalCid << std::endl;
      importDataForDeal(provider, kProposalCid, {});
      context->run();
    } catch (const boost::system::error_code &ec) {
      std::cerr << "Server cannot run: " + ec.message() << std::endl;
      return 1;
    } catch (...) {
      std::cerr << "Unknown error happened" << std::endl;
      return 1;
    }

    return 0;
  }

}  // namespace fc::markets::storage::example

int main() {
  return fc::markets::storage::example::main();
}
