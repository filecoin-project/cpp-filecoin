/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address/impl/address_builder_impl.hpp"

#include "crypto/blake2/blake2b160.hpp"
#include "primitives/address_codec.hpp"

namespace fc::primitives::address {

  using fc::primitives::Address;
  using Sec256k1PublicKey = libp2p::crypto::secp256k1::PublicKey;
  using BlsPublicKey = fc::crypto::bls::PublicKey;
  using fc::crypto::blake2b::blake2b_160;

  fc::outcome::result<Address> AddressBuilderImpl::makeFromSecp256k1PublicKey(
      Network network,
      const libp2p::crypto::secp256k1::PublicKey &public_key) noexcept {
    OUTCOME_TRY(hash, blake2b_160(public_key));
    std::vector<uint8_t> sec256k1_bytes{network, fc::primitives::SECP256K1};
    sec256k1_bytes.insert(sec256k1_bytes.end(), hash.begin(), hash.end());
    return fc::primitives::decode(sec256k1_bytes);
  }

  fc::outcome::result<Address> AddressBuilderImpl::makeFromBlsPublicKey(
      Network network, const crypto::bls::PublicKey &public_key) noexcept {
    std::vector<uint8_t> bls_bytes{network, fc::primitives::BLS};
    bls_bytes.insert(bls_bytes.end(), public_key.begin(), public_key.end());
    return fc::primitives::decode(bls_bytes);
  }

}  // namespace fc::primitives::address
