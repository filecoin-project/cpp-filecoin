/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_INDICES_MOCK_HPP
#define CPP_FILECOIN_INDICES_MOCK_HPP

#include <gmock/gmock.h>
#include "vm/indices/indices.hpp"

namespace fc::vm {
  class MockIndices : public Indices {
   public:
    MOCK_METHOD0(storagePowerConsensusMinMinerPower, fc::primitives::BigInt());

    MOCK_METHOD1(consensusPowerForStorageWeight,
                 fc::primitives::BigInt(actor::SectorStorageWeightDesc));
  };
}  // namespace fc::vm

#endif  // CPP_FILECOIN_INDICES_MOCK_HPP
