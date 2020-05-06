/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_MARKETS_PIECEIO_PIECEIO_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_MARKETS_PIECEIO_PIECEIO_MOCK_HPP

#include "markets/pieceio/pieceio_impl.hpp"

#include <gmock/gmock.h>

namespace fc::markets::pieceio {

  class PieceIOMock : public PieceIO {
   public:
    MOCK_METHOD3(generatePieceCommitment,
                 outcome::result<std::pair<CID, UnpaddedPieceSize>>(
                     const RegisteredProof &registered_proof,
                     const CID &payload_cid,
                     const Selector &selector));
  };

}  // namespace fc::markets::pieceio

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_MARKETS_PIECEIO_PIECEIO_MOCK_HPP
