/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_HASH_PROVIDER_HPP
#define CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_HASH_PROVIDER_HPP

#include "crypto/randomness/randomness_types.hpp"
#include "primitives/address/address.hpp"

namespace fc::crypto::vrf {
  /**
   * @class VRFHashProvider used to create VRFHash
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
    outcome::result<common::Hash256> create(DomainSeparationTag tag,
                                            const Address &miner_address,
                                            const Buffer &message);
  };

}  // namespace fc::crypto::vrf

#endif  // CPP_FILECOIN_CORE_CRYPTO_VRF_VRF_HASH_PROVIDER_HPP
