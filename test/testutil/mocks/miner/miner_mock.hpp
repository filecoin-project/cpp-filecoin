/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "miner/miner.hpp"

namespace fc::miner {

  class MinerMock : public Miner {
   public:
    MOCK_METHOD0(run, outcome::result<void>());

    MOCK_METHOD0(stop, void());

    MOCK_CONST_METHOD0(getAddress, Address());

    MOCK_CONST_METHOD1(
        getSectorInfo,
        outcome::result<std::shared_ptr<SectorInfo>>(SectorNumber));

    MOCK_METHOD3(addPieceToAnySector,
                 outcome::result<PieceAttributes>(UnpaddedPieceSize size,
                                                  const PieceData &piece_data,
                                                  DealInfo deal));

    MOCK_CONST_METHOD0(getSealing, std::shared_ptr<Sealing>());
  };

}  // namespace fc::miner
