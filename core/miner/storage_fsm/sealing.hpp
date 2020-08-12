/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_HPP

namespace fc::mining {
  // Epochs
  constexpr int kInteractivePoRepConfidence = 6;

  class Sealing {
   public:
    virtual ~Sealing() = default;
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_HPP
