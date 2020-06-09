/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/literals.hpp>
#include <libp2p/crypto/key.hpp>
#include "primitives/address/address.hpp"

namespace fc::markets::retrieval::test {
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  using libp2p::common::operator""_unhex;
  using primitives::address::Address;

  namespace config::provider {
    KeyPair keypair{
        PublicKey{
            {Key::Type::Ed25519,
             "5373b3cd50a00df1d6274872990e3b7a0b3e7461dbbeb411c82fda525b38a202"_unhex}},
        PrivateKey{
            {Key::Type::Ed25519,
             "fb2b5f11125f49230736087ae6af01c9460b77ed34b0f055447437aa9fa44cde"_unhex}}};
    std::string multiaddress{"/ip4/127.0.0.1/tcp/8080"};
    Address wallet{Address::makeFromId(1)};
  }  // namespace config::provider

  namespace config::client {
    KeyPair keypair{
        PublicKey{
            {Key::Type::Ed25519,
             "3329beb9ead4582e1a524225cde47ce959e0892841e3a8de7d3d09dd1edafda0"_unhex}},
        PrivateKey{
            {Key::Type::Ed25519,
             "322ee47e407e085bb15b136b8ab208857ce90aa471cbb79bcf9ac6de376b346b"_unhex}}};
  }
}  // namespace fc::markets::retrieval::test
