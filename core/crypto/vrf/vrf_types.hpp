/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_TYPES_HPP
#define CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_TYPES_HPP

#include "common/blob.hpp"
#include "crypto/bls/bls_types.hpp"

namespace fc::crypto::vrf {

  using VRFPublicKey = bls::PublicKey;
  using VRFSecretKey = bls::PrivateKey;
  using VRFProof = bls::Signature;
  using VRFResult = bls::Signature;
  using VRFHash = common::Hash256;

  /**
   * @brief VRF key pair definition
   */
  struct VRFKeyPair {
    VRFPublicKey public_key;
    VRFSecretKey secret_key;
  };

  enum class VRFError {
    MINER_ADDRESS_NOT_ID =
        1,                // miner address need to be id type to calculate hash
    VERIFICATION_FAILED,  // vrf verification failed
    SIGN_FAILED,          // vrf sing message failed
    ADDRESS_IS_NOT_BLS,   // vrf hash can be based only on bls
  };
}  // namespace fc::crypto::vrf

OUTCOME_HPP_DECLARE_ERROR(fc::crypto::vrf, VRFError)

#endif  // CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_TYPES_HPP
