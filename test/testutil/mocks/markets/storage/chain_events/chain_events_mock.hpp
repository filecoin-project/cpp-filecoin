/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "markets/storage/chain_events/chain_events.hpp"

#include <gmock/gmock.h>

namespace fc::markets::storage::chain_events {

  class ChainEventsMock : public ChainEvents {
   public:
    MOCK_METHOD3(onDealSectorCommitted,
                 void(const Address &, const DealId &, CommitCb));
  };

}  // namespace fc::markets::storage::chain_events
