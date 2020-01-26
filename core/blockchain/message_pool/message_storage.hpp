/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_BLOCKCHAIN_MESSAGE_POOL_MESSAGE_STORAGE_HPP
#define CPP_FILECOIN_BLOCKCHAIN_MESSAGE_POOL_MESSAGE_STORAGE_HPP

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/message/message.hpp"

namespace fc::blockchain::message_pool {

  using primitives::address::Address;
  using vm::message::SignedMessage;

  /**
   * Caches pending messages.
   */
  class MessageStorage {
   public:
    /**
     * Add message
     * @param message - message to add
     * @return error code in case of error, or nothing otherwise
     */
    virtual outcome::result<void> put(SignedMessage message) = 0;

    /**
     * Remove message
     * @param message - message to remove
     * @return error code in case of error, or nothing otherwise
     */
    virtual outcome::result<void> remove(SignedMessage message) = 0;

    /**
     * Get N top scored messages
     * @param n - how many messages return
     * @return no more than N top scored messages present in cache
     */
    virtual gsl::span<SignedMessage> getTopScored(size_t n) const = 0;
  };

}  // namespace fc::blockchain::message_pool

#endif  // CPP_FILECOIN_BLOCKCHAIN_MESSAGE_POOL_MESSAGE_STORAGE_HPP
