/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_EXAMPLE_PROVIDER_EXAMPLE_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_EXAMPLE_PROVIDER_EXAMPLE_HPP

#include <boost/asio/io_context.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include "api/api.hpp"
#include "crypto/bls/bls_provider.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "markets/pieceio/pieceio.hpp"
#include "markets/storage/provider.hpp"
#include "markets/storage/provider/stored_ask.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::markets::storage::example {

  using api::Api;
  using boost::asio::io_context;
  using crypto::bls::BlsProvider;
  using crypto::secp256k1::Secp256k1ProviderDefault;
  using libp2p::Host;
  using libp2p::multi::Multiaddress;
  using pieceio::PieceIO;
  using primitives::address::Address;
  using primitives::sector::RegisteredProof;
  using provider::Datastore;
  using provider::StorageProvider;
  using BlsKeyPair = fc::crypto::bls::KeyPair;

  outcome::result<std::shared_ptr<StorageProvider>> makeProvider(
      const Multiaddress &provider_multiaddress,
      const RegisteredProof &registered_proof,
      const BlsKeyPair &provider_keypair,
      const std::shared_ptr<BlsProvider> &bls_provider,
      const std::shared_ptr<Secp256k1ProviderDefault> &secp256k1_provider,
      const std::shared_ptr<Datastore> &datastore,
      const std::shared_ptr<PieceIO> piece_io,
      const std::shared_ptr<Host> &provider_host,
      const std::shared_ptr<io_context> &context,
      const std::shared_ptr<Api> &api,
      const Address &miner_actor_address);

}  // namespace fc::markets::storage::example

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_EXAMPLE_PROVIDER_EXAMPLE_HPP
