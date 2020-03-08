/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_BLOCKCHAIN_MESSAGE_POOL_GAS_PRICE_SCORED_MESSAGE_STORAGE_HPP
#define CPP_FILECOIN_BLOCKCHAIN_MESSAGE_POOL_GAS_PRICE_SCORED_MESSAGE_STORAGE_HPP

#include "filecoin/blockchain/message_pool/message_storage.hpp"

namespace fc::blockchain::message_pool {

  using primitives::address::Address;
  using vm::message::SignedMessage;

  /**
   * Compare messages by sender address and nonce
   */
  auto compareMessagesFunctor = [](const SignedMessage &lhs,
                                   const SignedMessage &rhs) {
    return (lhs.message.from < rhs.message.from)
           || ((lhs.message.from == rhs.message.from)
               && (lhs.message.nonce < rhs.message.nonce));
  };

  /**
   * Comparator based on gas price for scoring
   */
  auto compareGasFunctor = [](const SignedMessage &lhs,
                              const SignedMessage &rhs) {
    return (lhs.message.gasPrice > rhs.message.gasPrice)
           || ((lhs.message.gasPrice == rhs.message.gasPrice)
               && (compareMessagesFunctor(lhs, rhs)));
  };

  /**
   * Caches pending messages and order by gas price.
   */
  class GasPriceScoredMessageStorage : public MessageStorage {
   public:
    ~GasPriceScoredMessageStorage() override = default;

    /** \copydoc MessageStorage::put() */
    outcome::result<void> put(const SignedMessage &message) override;

    /** \copydoc MessageStorage::remove() */
    void remove(const SignedMessage &message) override;

    /** \copydoc MessageStorage::getTopScored() */
    std::vector<SignedMessage> getTopScored(size_t n) const override;

   private:
    std::set<SignedMessage, decltype(compareMessagesFunctor)> messages_{
        compareMessagesFunctor};
  };

}  // namespace fc::blockchain::message_pool

#endif  // CPP_FILECOIN_BLOCKCHAIN_MESSAGE_POOL_GAS_PRICE_SCORED_MESSAGE_STORAGE_HPP
