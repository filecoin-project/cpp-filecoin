/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_EXAMPLE_CLIENT_EXAMPLE_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_EXAMPLE_CLIENT_EXAMPLE_HPP

#include <boost/asio/io_context.hpp>
#include <libp2p/host/host.hpp>
#include "api/api.hpp"
#include "crypto/bls/bls_provider.hpp"
#include "crypto/bls/bls_types.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "markets/pieceio/pieceio.hpp"
#include "markets/storage/client/client.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::markets::storage::example {

  using api::Api;
  using boost::asio::io_context;
  using client::Client;
  using crypto::bls::BlsProvider;
  using crypto::secp256k1::Secp256k1ProviderDefault;
  using libp2p::Host;
  using pieceio::PieceIO;
  using primitives::sector::RegisteredProof;
  using BlsKeyPair = fc::crypto::bls::KeyPair;

  /**
   * Makes storage market client
   */
  outcome::result<std::shared_ptr<Client>> makeClient(
      const BlsKeyPair &client_keypair,
      const std::shared_ptr<BlsProvider> &bls_provider,
      const std::shared_ptr<Secp256k1ProviderDefault> &secp256k1_provider,
      const std::shared_ptr<PieceIO> &piece_io,
      const std::shared_ptr<libp2p::Host> &client_host,
      const std::shared_ptr<boost::asio::io_context> &context,
      const std::shared_ptr<Api> &api);

  /**
   * Sends get ask message to provider and prints result
   */
  void sendGetAsk(const std::shared_ptr<Client> &client,
                  const StorageProviderInfo &info);


}  // namespace fc::markets::storage::example

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_EXAMPLE_CLIENT_EXAMPLE_HPP
