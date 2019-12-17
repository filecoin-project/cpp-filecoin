/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address/impl/address_builder_impl.hpp"

#include "crypto/blake2/blake2b160.hpp"
#include "primitives/address_codec.hpp"

namespace fc::primitives::address {

  using primitives::Address;
  using Sec256k1PublicKey = libp2p::crypto::secp256k1::PublicKey;
  using BlsPublicKey = crypto::bls::PublicKey;
  using crypto::blake2b::blake2b_160;

  outcome::result<Address> AddressBuilderImpl::makeFromSecp256k1PublicKey(
      Network network, const Sec256k1PublicKey &public_key) noexcept {
    OUTCOME_TRY(hash, blake2b_160(public_key));
    std::vector<uint8_t> sec256k1_bytes{network, primitives::SECP256K1};
    sec256k1_bytes.insert(sec256k1_bytes.end(), hash.begin(), hash.end());
    return primitives::decode(sec256k1_bytes);
  }

  outcome::result<Address> AddressBuilderImpl::makeFromBlsPublicKey(
      Network network, const BlsPublicKey &public_key) noexcept {
    std::vector<uint8_t> bls_bytes{network, primitives::BLS};
    bls_bytes.insert(bls_bytes.end(), public_key.begin(), public_key.end());
    return primitives::decode(bls_bytes);
  }

}  // namespace fc::primitives::address
