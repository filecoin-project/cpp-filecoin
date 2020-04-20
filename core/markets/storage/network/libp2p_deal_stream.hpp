/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_NETWORKLIBP2P_DEAL_STREAM_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_NETWORKLIBP2P_DEAL_STREAM_HPP

#include <libp2p/connection/stream.hpp>
#include <libp2p/peer/peer_id.hpp>
#include "common/logger.hpp"
#include "markets/storage/deal_stream.hpp"

namespace fc::markets::storage {

  using libp2p::connection::Stream;
  using libp2p::peer::PeerId;

  class Libp2pDealStream : public StorageDealStream {
    static constexpr size_t buffer_size{1024};

   public:
    Libp2pDealStream(PeerId peer_id, std::shared_ptr<Stream> stream);

    auto readDealProposal() -> outcome::result<Proposal> override;

    auto writeDealProposal(const Proposal &proposal)
        -> outcome::result<void> override;

    auto readDealResponse() -> outcome::result<SignedResponse> override;

    auto writeDealResponse(const SignedResponse &response)
        -> outcome::result<void> override;

    auto remotePeer() const -> PeerId override;

    auto close() -> outcome::result<void> override;

   private:
    PeerId peer_id_;
    std::shared_ptr<Stream> stream_;
    common::Logger logger_ = common::createLogger("AskStream");
  };

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_MARKETS_STORAGE_NETWORKLIBP2P_DEAL_STREAM_HPP
