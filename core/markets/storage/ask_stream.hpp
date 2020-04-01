/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE__NETWORK_STORAGE_ASK_STREAM_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE__NETWORK_STORAGE_ASK_STREAM_HPP

#include "common/outcome.hpp"
#include "markets/storage/ask_protocol.hpp"

namespace fc::markets::storage {

  /**
   * StorageAskStream is a stream for reading/writing requests & responses on
   * the Storage Ask protocol
   */
  class StorageAskStream {
   public:
    virtual ~StorageAskStream() = default;

    virtual auto readAskRequest() -> outcome::result<AskRequest> = 0;

    virtual auto writeAskRequest(AskRequest) -> outcome::result<void> = 0;

    virtual auto readAskResponse() -> outcome::result<AskResponse> = 0;

    virtual auto writeAskResponse(AskResponse) -> outcome::result<void> = 0;

    virtual auto close() -> outcome::result<void> = 0;
  };

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE__NETWORK_STORAGE_ASK_STREAM_HPP
