/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP
#define CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP

#include "crypto/vrf/vrf_types.hpp"

namespace fc::crypto::vrf {

  /**
   * @class VRFProvider provides VRF functionality according to
   * https://github.com/filecoin-project/lotus/blob/1914412adf3c81028fcc305b887ca8ad189bc2dc/chain/gen/gen.go#L597
   */
  class VRFProvider {
   public:
    using Address = primitives::address::Address;
    using Buffer = common::Buffer;

    virtual ~VRFProvider() = default;

    /**
     * @brief calculates vrf signature
     * @param secret_key worker's secret key
     * @param params vrf parameters
     * @return generated signature or error
     */
    virtual outcome::result<VRFResult> computeVRF(
        const VRFSecretKey &secret_key, const VRFParams &params) const = 0;

    /**
     * @brief verifies message against signature and public key
     * @param public_key worker's public key
     * @param params vrf parameters
     * @param proof vrf proof
     * @return result of verification or error
     */
    virtual outcome::result<bool> verifyVRF(const VRFPublicKey &public_key,
                                            const VRFParams &params,
                                            const VRFProof &proof) const = 0;
  };

}  // namespace fc::crypto::vrf

#endif  // CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP
