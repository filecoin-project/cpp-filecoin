/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_BLOCKCHAIN_MESSAGE_POOL_GAS_PRICE_SCORED_MESSAGE_STORAGE_HPP
#define CPP_FILECOIN_BLOCKCHAIN_MESSAGE_POOL_GAS_PRICE_SCORED_MESSAGE_STORAGE_HPP

#include "blockchain/message_pool/message_storage.hpp"

namespace fc::blockchain::message_pool {

  using primitives::address::Address;
  using vm::message::SignedMessage;

  /**
   * Caches pending messages and order by gas price.
   */
  class GasPriceScoredMessageStorage {
   public:
    /** \copydoc MessageStorage::put() */
    outcome::result<void> put(SignedMessage message) override;

    /** \copydoc MessageStorage::remove() */
    outcome::result<void> remove(SignedMessage message) override;

    /** \copydoc MessageStorage::getTopScored() */
    gsl::span<SignedMessage> getTopScored(size_t n) const override;

   private:
//    std::set<> messages;
  };

}  // namespace fc::blockchain::message_pool

#endif  // CPP_FILECOIN_BLOCKCHAIN_MESSAGE_POOL_GAS_PRICE_SCORED_MESSAGE_STORAGE_HPP
