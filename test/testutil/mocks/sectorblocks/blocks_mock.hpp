/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "sectorblocks/blocks.hpp"

namespace fc::sectorblocks {

  class SectorBlocksMock : public SectorBlocks {
   public:
    MOCK_METHOD3(addPiece,
                 outcome::result<PieceAttributes>(UnpaddedPieceSize,
                                                  const std::string &,
                                                  DealInfo));
    MOCK_CONST_METHOD1(getRefs, outcome::result<std::vector<PieceLocation>>(DealId));

    MOCK_CONST_METHOD0(getMiner, std::shared_ptr<Miner>());
  };

}  // namespace fc::sectorblocks
