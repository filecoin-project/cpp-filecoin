/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_DRAND_CLIENT_HPP
#define CPP_FILECOIN_CORE_DRAND_CLIENT_HPP

#include "drand/messages.hpp"

namespace fc::drand {

  /**
   * A client to public API for the Drand chain node
   */
  class DrandSyncClient {
   public:
    virtual ~DrandSyncClient() = default;

    /**
     * Requests randomness value generated in the Drand chain round
     * @param round - round number to read the value. Could be 0 ZERO to get the
     * latest value.
     * @return a response with on-chain-generated randomness value
     */
    virtual outcome::result<PublicRandResponse> publicRand(uint64_t round) = 0;

    /*
     * Not documented in Drand sources but used in Lotus.
     * Should be the same as non-stream version by meaning but returning
     * multiple values.
     */
    virtual outcome::result<std::vector<PublicRandResponse>> publicRandStream(
        uint64_t round) = 0;

    /**
     * Get the group description that the Drand endpoint belongs to
     * @return group packet message
     */
    virtual outcome::result<GroupPacket> group() = 0;
  };
}  // namespace fc::drand

#endif  // CPP_FILECOIN_CORE_DRAND_CLIENT_HPP
