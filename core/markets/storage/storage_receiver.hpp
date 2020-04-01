/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_RECEIVER_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_RECEIVER_HPP

#include "markets/storage/ask_stream.hpp"
#include "markets/storage/deal_stream.hpp"

namespace fc::markets::storage {

  /**
   * StorageReceiver implements functions for receiving incoming data on storage
   * protocols
   */
  class StorageReceiver {
   public:
    virtual ~StorageReceiver() = default;

    virtual void handleAskStream(const StorageAskStream &stream) = 0;

    virtual void handleDealStream(const StorageDealStream &stream) = 0;
  };

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_RECEIVER_HPP
