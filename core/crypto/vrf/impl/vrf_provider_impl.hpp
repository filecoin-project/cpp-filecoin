/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_VRF_IMPL_VRF_PROVIDER_IMPL_HPP
#define CPP_FILECOIN_CORE_CRYPTO_VRF_IMPL_VRF_PROVIDER_IMPL_HPP

#include "crypto/bls/bls_provider.hpp"
#include "crypto/vrf/vrf_provider.hpp"

namespace fc::crypto::vrf {

  class VRFProviderImpl : public VRFProvider {
   public:
    ~VRFProviderImpl() override = default;

    explicit VRFProviderImpl(std::shared_ptr<bls::BlsProvider> bls_provider);

    outcome::result<VRFResult> computeVRF(
        const VRFSecretKey &secret_key,
        const VRFHash &msg) const override;

    outcome::result<bool> verifyVRF(const VRFPublicKey &public_key,
                                    const VRFHash &msg,
                                    const VRFProof &vrf_proof) const override;

   private:
    std::shared_ptr<bls::BlsProvider> bls_provider_;  ///< bls provider instance
  };

}  // namespace fc::crypto::vrf

#endif  // CPP_FILECOIN_CORE_CRYPTO_VRF_IMPL_VRF_PROVIDER_IMPL_HPP
