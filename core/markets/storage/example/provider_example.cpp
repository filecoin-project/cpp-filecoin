/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <libp2p/peer/peer_info.hpp>
#include "api/api.hpp"
#include "markets/pieceio/pieceio.hpp"
#include "markets/storage/provider/provider.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"

namespace fc::markets::storage::example {

  using api::Api;
  using api::MinerApi;
  using common::Buffer;
  using crypto::bls::BlsProvider;
  using crypto::secp256k1::Secp256k1ProviderDefault;
  using fc::storage::keystore::InMemoryKeyStore;
  using fc::storage::keystore::KeyStore;
  using libp2p::multi::Multiaddress;
  using pieceio::PieceIO;
  using primitives::address::Address;
  using primitives::sector::RegisteredProof;
  using provider::Datastore;
  using provider::StorageProvider;
  using provider::StorageProviderImpl;
  using BlsKeyPair = fc::crypto::bls::KeyPair;

  outcome::result<std::shared_ptr<StorageProviderImpl>> makeProvider(
      const Multiaddress &provider_multiaddress,
      const RegisteredProof &registered_proof,
      const BlsKeyPair &provider_keypair,
      const std::shared_ptr<BlsProvider> &bls_provider,
      const std::shared_ptr<Secp256k1ProviderDefault> &secp256k1_provider,
      const std::shared_ptr<Datastore> &datastore,
      const std::shared_ptr<PieceIO> piece_io,
      const std::shared_ptr<libp2p::Host> &provider_host,
      const std::shared_ptr<boost::asio::io_context> &context,
      const std::shared_ptr<Api> &api,
      const std::shared_ptr<MinerApi> &miner_api,
      const Address &miner_actor_address) {
    OUTCOME_TRY(provider_host->listen(provider_multiaddress));

    std::shared_ptr<KeyStore> keystore =
        std::make_shared<InMemoryKeyStore>(bls_provider, secp256k1_provider);

    Address bls_address = Address::makeBls(provider_keypair.public_key);
    OUTCOME_TRY(keystore->put(bls_address, provider_keypair.private_key));

    std::shared_ptr<StorageProviderImpl> provider =
        std::make_shared<StorageProviderImpl>(registered_proof,
                                              provider_host,
                                              context,
                                              keystore,
                                              datastore,
                                              api,
                                              miner_api,
                                              miner_actor_address,
                                              piece_io);
    provider->init();
    return provider;
  }

}  // namespace fc::markets::storage::example
