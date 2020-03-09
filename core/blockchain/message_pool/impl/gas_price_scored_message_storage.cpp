/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/blockchain/message_pool/impl/gas_price_scored_message_storage.hpp"

#include "filecoin/blockchain/message_pool/message_pool_error.hpp"

using fc::blockchain::message_pool::GasPriceScoredMessageStorage;
using fc::blockchain::message_pool::MessagePoolError;
using fc::vm::message::SignedMessage;

fc::outcome::result<void> GasPriceScoredMessageStorage::put(
    const SignedMessage &message) {
  auto res = messages_.insert(message);
  if (!res.second) return MessagePoolError::MESSAGE_ALREADY_IN_POOL;
  return fc::outcome::success();
}

void GasPriceScoredMessageStorage::remove(const SignedMessage &message) {
  messages_.erase(message);
}

std::vector<SignedMessage> GasPriceScoredMessageStorage::getTopScored(
    size_t n) const {
  std::set<SignedMessage, decltype(compareGasFunctor)> score(
      messages_.begin(), messages_.end(), compareGasFunctor);
  return std::vector<SignedMessage>(
      score.begin(),
      (n < score.size()) ? std::next(score.begin(), n) : score.end());
}
