/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_INDICES_INDICES_MOCK_HPP
#define CPP_FILECOIN_VM_INDICES_INDICES_MOCK_HPP

#include <gmock/gmock.h>
#include "vm/indices/indices.hpp"

namespace fc::vm::indices {

  class MockIndices : public Indices {
   public:
    MOCK_METHOD0(storagePowerConsensusMinMinerPower, fc::power::Power());

    MOCK_METHOD1(consensusPowerForStorageWeight,
                 fc::power::Power(SectorStorageWeightDesc));
  };

}  // namespace fc::vm::indices

#endif  // CPP_FILECOIN_VM_INDICES_INDICES_MOCK_HPP
