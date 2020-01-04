/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_HASH_PROVIDER_HPP
#define CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_HASH_PROVIDER_HPP

#include "crypto/randomness/randomness_types.hpp"
#include "crypto/vrf/vrf_types.hpp"
#include "primitives/address/address.hpp"

namespace fc::crypto::vrf {
  /**
   * @class VRFHashProvider used to create VRFHash according to lotus hash implementation:
   * https://github.com/filecoin-project/lotus/blob/1914412adf3c81028fcc305b887ca8ad189bc2dc/chain/gen/gen.go#L579
   */
  class VRFHashProvider {
   public:
    using Buffer = common::Buffer;
    using Address = primitives::address::Address;
    using DomainSeparationTag = randomness::DomainSeparationTag;

    /**
     * @brief creates vrf hash to be used in vrf sign or verify methods
     * @param tag domain separation (personalization) tag
     * @param miner_address miner address
     * @param message data to hash
     * @return vrf hash value
     */
    static outcome::result<VRFHash> create(DomainSeparationTag tag,
                                           const Address &miner_address,
                                           const Buffer &message);
  };

}  // namespace fc::crypto::vrf

#endif  // CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_HASH_PROVIDER_HPP
