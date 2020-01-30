/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address/impl/address_builder_impl.hpp"

#include "crypto/blake2/blake2b160.hpp"
#include "primitives/address/address_codec.hpp"

namespace fc::primitives::address {

  using primitives::address::Address;
  using Sec256k1PublicKey = libp2p::crypto::secp256k1::PublicKey;
  using BlsPublicKey = crypto::bls::PublicKey;
  using crypto::blake2b::blake2b_160;
  using primitives::address::BLSPublicKeyHash;
  using primitives::address::Protocol;
  using primitives::address::Secp256k1PublicKeyHash;

  outcome::result<Address> AddressBuilderImpl::makeFromSecp256k1PublicKey(
      Network network, const Sec256k1PublicKey &public_key) noexcept {
    auto hash = blake2b_160(public_key);
    Secp256k1PublicKeyHash secp256k1Hash{hash};
    return Address{network, secp256k1Hash};
  }

  outcome::result<Address> AddressBuilderImpl::makeFromBlsPublicKey(
      Network network, const BlsPublicKey &public_key) noexcept {
    BLSPublicKeyHash blsHash{public_key};
    return Address{network, blsHash};
  }

}  // namespace fc::primitives::address
