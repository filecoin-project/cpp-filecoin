/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP
#define CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP

#include "common/blob.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "crypto/vrf/vrf_types.hpp"
#include "primitives/address/address.hpp"

namespace fc::crypto::vrf {

  enum class VRFProviderError {
    MINER_CLASS_NOT_ID = 1,

  };

  /**
   * @class VRFProvider supplies with methods for generation and verifying VRF
   */
  class VRFProvider {
   public:
    using Address = primitives::address::Address;
    using Buffer = common::Buffer;

    virtual ~VRFProvider() = default;

    /**
     * @brief generates vrf based on personalization, miner address and message
     * @param tag personalization tag
     * @param miner address
     * @param miner address
     * @param message message
     * @return vrf result
     */
    virtual outcome::result<VRFResult> generateVRF(
        randomness::DomainSeparationTag tag,
        const VRFSecretKey &worker_secret_key,
        const Buffer &miner_bytes,
        const Buffer &message) const = 0;

    // TODO(yuraz) specify entities in description
    /**
     * @brief verifies *** against ***
     * @param tag personalization tag
     * * @param worker address
     * @param miner address
     * @param message message
     * @param vrf_proof proof
     * @return true of false if check succeeded or error otherwise
     */
    virtual outcome::result<bool> verifyVRF(
        randomness::DomainSeparationTag tag,
        const VRFPublicKey &worker_public_key,
        const Buffer &miner_bytes,
        const Buffer &message,
        const VRFProof &vrf_proof) const = 0;
  };

}  // namespace fc::crypto::vrf

#endif  // CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_PROVIDER_HPP
