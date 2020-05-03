/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/provider.hpp"

#include "markets/storage/stored_ask.hpp"

namespace fc::markets::storage::provider {

  outcome::result<void> StorageProviderImpl::addAsk(const TokenAmount &price,
                                                    ChainEpoch duration) {
    return stored_ask_->addAsk(price, duration);
  }

  outcome::result<std::vector<std::shared_ptr<SignedStorageAsk>>>
  StorageProviderImpl::listAsks(const Address &address) {
    std::vector<std::shared_ptr<SignedStorageAsk>> result;
    OUTCOME_TRY(signed_storage_ask, stored_ask_->getAsk(address));
    if (signed_storage_ask) {
      result.push_back(*signed_storage_ask);
    }
    return result;
  }
}  // namespace fc::markets::storage::provider
