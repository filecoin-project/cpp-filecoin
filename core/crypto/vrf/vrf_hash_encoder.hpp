/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_HASH_ENCODER_HPP
#define CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_HASH_ENCODER_HPP

#include "crypto/vrf/vrf_types.hpp"

namespace fc::crypto::vrf {

  /**
   * @brief encodeVrfParams function used to create VRFHash according to lotus
   * hash implementation:
   * https://github.com/filecoin-project/lotus/blob/1914412adf3c81028fcc305b887ca8ad189bc2dc/chain/gen/gen.go#L579
   * @param params vrf parameters
   * @return hash value or error
   */
  outcome::result<VRFHash> encodeVrfParams(const VRFParams &params);
}  // namespace fc::crypto::vrf

#endif  // CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_HASH_ENCODER_HPP
