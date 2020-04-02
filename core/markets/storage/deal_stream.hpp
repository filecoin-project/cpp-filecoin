/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE__NETWORK_STORAGE_DEAL_STREAM_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE__NETWORK_STORAGE_DEAL_STREAM_HPP

#include "common/outcome.hpp"
#include "markets/storage/deal_protocol.hpp"

namespace fc::markets::storage {

  /**
   * StorageDealStream is a stream for reading and writing requests and
   * responses on the storage deal protocol
   */
  class StorageDealStream {
   public:
    virtual ~StorageDealStream() = default;

    virtual auto readDealProposal() -> outcome::result<Proposal> = 0;

    virtual auto writeDealProposal(Proposal) -> outcome::result<void> = 0;

    virtual auto readDealResponse() -> outcome::result<SignedResponse> = 0;

    virtual auto writeDealResponse(SignedResponse) -> outcome::result<void> = 0;

    virtual auto remotePeer() -> PeerId = 0;

    virtual auto close() -> outcome::result<void> = 0;
  }

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE__NETWORK_STORAGE_DEAL_STREAM_HPP
