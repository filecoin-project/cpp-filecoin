/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/example/client_example.hpp"

#include <iostream>
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/client/client_impl.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"

namespace fc::markets::storage::example {

  using api::Api;
  using client::ClientImpl;
  using crypto::bls::BlsProvider;
  using crypto::secp256k1::Secp256k1ProviderDefault;
  using fc::storage::keystore::InMemoryKeyStore;
  using fc::storage::keystore::KeyStore;
  using libp2p::multi::HashType;
  using libp2p::multi::MulticodecType;
  using libp2p::multi::Multihash;
  using pieceio::PieceIO;
  using primitives::ChainEpoch;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::RegisteredProof;
  using BlsKeyPair = fc::crypto::bls::KeyPair;

  outcome::result<std::shared_ptr<Client>> makeClient(
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
    OUTCOME_TRY(keystore->put(bls_address, client_keypair.private_key));

    auto client = std::make_shared<ClientImpl>(
        client_host, context, api, keystore, piece_io);
    client->init();
    return client;
  }

  void sendGetAsk(const std::shared_ptr<Client> &client,
                  const StorageProviderInfo &info) {
    client->getAsk(info, [](outcome::result<SignedStorageAsk> ask_res) {
      if (ask_res.has_error()) {
        std::cout << "Response error " << ask_res.error().message()
                  << std::endl;
      } else {
        std::cout << "Response read price " << ask_res.value().ask.price
                  << std::endl;
        std::cout << "Response read expiry " << ask_res.value().ask.expiry
                  << std::endl;
      }
    });
  }

}  // namespace fc::markets::storage::example
