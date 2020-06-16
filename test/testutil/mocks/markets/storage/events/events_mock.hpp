/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_MARKETS_STORAGE_EVENTS_EVENTS_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_MARKETS_STORAGE_EVENTS_EVENTS_MOCK_HPP

#include "markets/storage/events/events.hpp"

#include <gmock/gmock.h>

namespace fc::markets::storage::events {

  class EventsMock : public Events {
   public:
    MOCK_METHOD0(run, outcome::result<void>());
    MOCK_METHOD0(stop, void());
    MOCK_METHOD2(onDealSectorCommitted,
                 std::shared_ptr<PromiseResult>(const Address &,
                                                const DealId &));
  };

}  // namespace fc::markets::storage::events

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_MARKETS_STORAGE_EVENTS_EVENTS_MOCK_HPP
