/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_MARKETS_STORAGE_EVENTS_EVENTS_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_MARKETS_STORAGE_EVENTS_EVENTS_MOCK_HPP

#include "markets/storage/chain_events/chain_events.hpp"

#include <gmock/gmock.h>

namespace fc::markets::storage::chain_events {

  class ChainEventsMock : public ChainEvents {
   public:
    MOCK_METHOD3(onDealSectorCommitted,
                 void(const Address &, const DealId &, Cb));
  };

}  // namespace fc::markets::storage::chain_events

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_MARKETS_STORAGE_EVENTS_EVENTS_MOCK_HPP
