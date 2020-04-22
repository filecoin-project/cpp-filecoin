/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_RECEIVER_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_RECEIVER_HPP

#include "common/libp2p/cbor_stream.hpp"

namespace fc::markets::storage {

  using common::libp2p::CborStream;

  /**
   * StorageReceiver implements functions for receiving incoming data on storage
   * protocols
   */
  class StorageReceiver {
   public:
    virtual ~StorageReceiver() = default;

    virtual void handleAskStream(const std::shared_ptr<CborStream> &stream) = 0;

    virtual void handleDealStream(
        const std::shared_ptr<CborStream> &stream) = 0;
  };

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_STORAGE_RECEIVER_HPP
