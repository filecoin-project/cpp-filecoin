/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/tipset_cache.hpp"

#include <gmock/gmock.h>

namespace fc::mining {
  class TipsetCacheMock : public TipsetCache {
   public:
    MOCK_METHOD1(add, outcome::result<void>(const Tipset &));

    MOCK_METHOD1(revert, outcome::result<void>(const Tipset &));

    MOCK_METHOD1(getNonNull, outcome::result<Tipset>(ChainEpoch));

    MOCK_METHOD1(get, outcome::result<boost::optional<Tipset>>(ChainEpoch));

    MOCK_CONST_METHOD0(best, outcome::result<Tipset>());
  };
}  // namespace fc::mining
