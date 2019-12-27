/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_VRF_IMPL_VRF_PROVIDER_IMPL_HPP
#define CPP_FILECOIN_CORE_CRYPTO_VRF_IMPL_VRF_PROVIDER_IMPL_HPP

#include "crypto/vrf/vrf_provider.hpp"
#include "crypto/bls/bls_provider.hpp"

namespace fc::crypto::vrf {

  class VRFProviderImpl : public VRFProvider {
   public:
    ~VRFProviderImpl() override = default;

    explicit VRFProviderImpl(std::shared_ptr<bls::BlsProvider> bls_provider);

    outcome::result<VRFResult> generateVRF(
        randomness::DomainSeparationTag tag,
        const Address &miner,
        const Buffer &message) const override;

    virtual outcome::result<bool> verifyVRF(
        randomness::DomainSeparationTag tag,
        const Address &miner,
        const Buffer &message,
        const VRFProof &vrf_proof) const override;

   private:
    // calculates vrf hash
    outcome::result<common::Hash256> vrfHash(
        randomness::DomainSeparationTag tag,
        const Address &miner,
        const Buffer &message) const;

    std::shared_ptr<bls::BlsProvider> bls_provider_; ///< bls provider instance
  };

}  // namespace fc::crypto::vrf

#endif  // CPP_FILECOIN_CORE_CRYPTO_VRF_IMPL_VRF_PROVIDER_IMPL_HPP
