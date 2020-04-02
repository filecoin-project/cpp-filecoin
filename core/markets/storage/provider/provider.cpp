/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/provider.hpp"

#include "markets/storage/stored_ask.hpp"

namespace fc::markets::storage {

  outcome::result<void> StorageProviderImpl::addAsk(const TokenAmount &price,
                                                    ChainEpoch duration) {
    return outcome::success();
  }
}  // namespace fc::markets::storage
