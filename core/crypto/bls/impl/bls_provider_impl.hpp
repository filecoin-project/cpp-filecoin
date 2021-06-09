/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/metrics/instance_count.hpp>

#include "crypto/bls/bls_provider.hpp"

namespace fc::crypto::bls {
  class BlsProviderImpl : public BlsProvider {
   public:
    outcome::result<KeyPair> generateKeyPair() const override;

    outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &key) const override;

    outcome::result<Signature> sign(gsl::span<const uint8_t> message,
                                    const PrivateKey &key) const override;

    outcome::result<bool> verifySignature(gsl::span<const uint8_t> message,
                                          const Signature &signature,
                                          const PublicKey &key) const override;

    outcome::result<Signature> aggregateSignatures(
        gsl::span<const Signature> signatures) const override;

   private:
    /**
     * @brief Generate BLS message digest
     * @param message - data for hashing
     * @return BLS digest or error code
     */
    static outcome::result<Digest> generateHash(
        gsl::span<const uint8_t> message);

    LIBP2P_METRICS_INSTANCE_COUNT(fc::crypto::bls::BlsProviderImpl);
  };
}  // namespace fc::crypto::bls
