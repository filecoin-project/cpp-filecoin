/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP
#define CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP

#include "common/blob.hpp"
#include "crypto/vrf/vrf_types.hpp"
#include "crypto/vrf/vrfhash_provider.hpp"
#include "primitives/address/address.hpp"

namespace fc::crypto::vrf {

  /**
   * @class VRFProvider supplies with methods for generation and verifying VRF
   */
  class VRFProvider {
   public:
    using Address = primitives::address::Address;
    using Buffer = common::Buffer;

    virtual ~VRFProvider() = default;

    /**
     * @brief generates vrf signature
     * @param worker_secret_key secret key
     * @param msg hashed message
     * @return generated signature or error
     */
    virtual outcome::result<VRFResult> generateVRF(
        const VRFSecretKey &worker_secret_key, const VRFHash &msg) const = 0;

    /**
     * @brief verifies message against signature and public key
     * @param worker_public_key worker public key
     * @param msg hashed message
     * @param vrf_proof proof
     * @return result of verification or error
     */
    virtual outcome::result<bool> verifyVRF(
        const VRFPublicKey &worker_public_key,
        const VRFHash &msg,
        const VRFProof &vrf_proof) const = 0;
  };

}  // namespace fc::crypto::vrf

#endif  // CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP
