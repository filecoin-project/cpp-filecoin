/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/retrieval/provider/retrieval_handler/retrieval_handler_impl.hpp"

namespace fc::markets::retrieval::provider {
  RetrievalHandlerImpl::RetrievalHandlerImpl(PieceIOShPtr piece_io)
      : piece_io_{std::move(piece_io)} {}

  void RetrievalHandlerImpl::onNewRequest(std::shared_ptr<CborStream> stream) {}
}  // namespace fc::markets::retrieval::provider
