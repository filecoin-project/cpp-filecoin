/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/precommit_policy.hpp"

#include <gmock/gmock.h>

namespace fc::mining {
  class PreCommitPolicyMock : public PreCommitPolicy {
   public:
    MOCK_METHOD1(expiration, outcome::result<ChainEpoch>(gsl::span<const types::Piece>));
  };
}  // namespace fc::mining
