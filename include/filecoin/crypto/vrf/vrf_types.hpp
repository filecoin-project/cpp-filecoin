/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_TYPES_HPP
#define CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_TYPES_HPP

#include "filecoin/common/blob.hpp"
#include "filecoin/common/buffer.hpp"
#include "filecoin/crypto/bls/bls_types.hpp"
#include "filecoin/crypto/randomness/randomness_types.hpp"
#include "filecoin/primitives/address/address.hpp"

namespace fc::crypto::vrf {

  /**
   * @brief vrf public key type
   */
  using VRFPublicKey = bls::PublicKey;
  /**
   * @brief vrf secret key type
   */
  using VRFSecretKey = bls::PrivateKey;
  /**
   * @brief vrf proof value
   */
  using VRFProof = bls::Signature;

  /**
   * @brief result of computing vrf
   */
  using VRFResult = bls::Signature;

  /**
   * @brief return value of vrf hash encode
   */
  using VRFHash = common::Hash256;

  /**
   * @brief vrf parameters structure
   */
  struct VRFParams {
    randomness::DomainSeparationTag personalization_tag;
    primitives::address::Address miner_address;
    common::Buffer message;
  };

  /**
   * @brief VRF key pair definition
   */
  struct VRFKeyPair {
    VRFPublicKey public_key;
    VRFSecretKey secret_key;
  };

  /**
   * @brief vrf errors enumeration
   */
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
