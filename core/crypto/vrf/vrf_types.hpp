/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_TYPES_HPP
#define CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_TYPES_HPP

#include "common/blob.hpp"
#include "crypto/bls/bls_types.hpp"

namespace fc::crypto::vrf {
  using VRFOutput = common::Hash256;

  using VRFPublicKey = bls::PublicKey;
  using VRFSecretKey = bls::PrivateKey;
  using VRFProof = bls::Signature;
  using VRFResult = bls::Signature;

  /**
   * @brief VRF key pair definition
   */
  struct VRFKeyPair {
    VRFPublicKey public_key;
    VRFSecretKey secret_key;
  };

  /**
   * @brief hash personalization value
   */
  enum HashPersonalization : uint64_t { DSepTicket = 1, DSepElectionPost = 2 };

  enum class VRFError {
    MINER_ADDRESS_NOT_ID = 1, // miner address need to be id type to calculate hash
  };
}  // namespace fc::crypto::vrf

OUTCOME_HPP_DECLARE_ERROR(fc::crypto::vrf, VRFError)

#endif  // CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_TYPES_HPP
