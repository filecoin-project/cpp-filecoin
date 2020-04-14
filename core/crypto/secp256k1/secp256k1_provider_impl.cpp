/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/secp256k1/secp256k1_provider_impl.hpp"

/**
 * libsecp256k1 build parameters
 */
#define USE_NUM_NONE
#define USE_FIELD_10X26
#define USE_FIELD_INV_BUILTIN
#define USE_SCALAR_8X32
#define USE_SCALAR_INV_BUILTIN
#define NDEBUG
#define ENABLE_MODULE_RECOVERY

#include "../../../deps/libsecp256k1/src/secp256k1.h"
#include "../../../deps/libsecp256k1/src/secp256k1_recovery.h"

//#include "libsecp256k1/src/secp256k1.h"

namespace fc::crypto::secp256k1 {

  outcome::result<PublicKey> Secp256k1ProviderImpl::recoverPublicKey(
      gsl::span<const uint8_t> message, const Signature &signature) const {
    // TODO free context
    secp256k1_context *ctx = secp256k1_context_create(
        SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    secp256k1_ecdsa_recoverable_signature sig;
    secp256k1_pubkey pubkey;

    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(
            ctx, &sig, signature.data(), (int)signature.data()[64])) {
      // TODO
      return libp2p::crypto::CryptoProviderError::INVALID_KEY_TYPE;
    }
    if (!secp256k1_ecdsa_recover(ctx, &pubkey, &sig, message.data())) {
      // TODO
      return libp2p::crypto::CryptoProviderError::INVALID_KEY_TYPE;
    }
    std::array<unsigned char, 65> pubkey_out;
    size_t outputlen = 65;
    if (secp256k1_ec_pubkey_serialize(ctx,
                                      pubkey_out.data(),
                                      &outputlen,
                                      &pubkey,
                                      SECP256K1_EC_UNCOMPRESSED)
        != 0) {
      // TODO
      return libp2p::crypto::CryptoProviderError::INVALID_KEY_TYPE;
    }

    // TODO
    return libp2p::crypto::CryptoProviderError::INVALID_KEY_TYPE;
  }

}  // namespace fc::crypto::secp256k1
