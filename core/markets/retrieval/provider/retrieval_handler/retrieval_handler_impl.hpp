/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_HANDLER_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_HANDLER_IMPL_HPP

#include "common/libp2p/cbor_stream.hpp"
#include "markets/pieceio/pieceio.hpp"

namespace fc::markets::retrieval::provider {
  class RetrievalHandlerImpl {
   protected:
    using CborStream = common::libp2p::CborStream;
    using PieceIOShPtr = std::shared_ptr<pieceio::PieceIO>;

   public:
    RetrievalHandlerImpl(PieceIOShPtr piece_io);

    void onNewRequest(std::shared_ptr<CborStream> stream);

   private:
    PieceIOShPtr piece_io_;
  };
}  // namespace fc::markets::retrieval::provider

#endif
