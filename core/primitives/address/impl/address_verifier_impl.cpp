/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address/impl/address_verifier_impl.hpp"

#include <libp2p/crypto/secp256k1_types.hpp>

#include "common/visitor.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "crypto/bls_provider/bls_types.hpp"
#include "primitives/address/address_verifier.hpp"

namespace fc::primitives::address {

  using Sec256k1PublicKey = libp2p::crypto::secp256k1::PublicKey;
  using BlsPublicKey = fc::crypto::bls::PublicKey;

  fc::outcome::result<bool> AddressVerifierImpl::verifySyntax(
      const fc::primitives::Address &address,
      gsl::span<const uint8_t> seed_data) noexcept {
    return visit_in_place(
        address.data,
        [](uint64_t v) { return true; },
        [&seed_data](
            const Secp256k1PublicKeyHash &v) -> fc::outcome::result<bool> {
          if (seed_data.size() != std::tuple_size<Sec256k1PublicKey>::value)
            return false;
          // seed_data is a blake2b-160 hash of a public key
          OUTCOME_TRY(hash, fc::crypto::blake2b::blake2b_160(seed_data));
          return std::equal(v.begin(), v.end(), hash.begin());
        },
        [&seed_data](const ActorExecHash &v) -> fc::outcome::result<bool> {
          OUTCOME_TRY(hash, fc::crypto::blake2b::blake2b_160(seed_data));
          return std::equal(hash.begin(), hash.end(), v.begin());
        },
        [&seed_data](const BLSPublicKeyHash &v) {
          if (seed_data.size() != std::tuple_size<BlsPublicKey>::value)
            return false;
          // seed_data is a public key
          return std::equal(v.begin(), v.end(), seed_data.begin());
        });
  }

}  // namespace fc::primitives::address
