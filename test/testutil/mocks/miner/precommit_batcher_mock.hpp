/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include "miner/storage_fsm/precommit_batcher.hpp"

namespace fc::mining {

  class PreCommitBatcherMock : public PreCommitBatcher {
   public:
    MOCK_METHOD4(addPreCommit,
                 outcome::result<void>(const SectorInfo &,
                                       const TokenAmount &,
                                       const SectorPreCommitInfo &,
                                       const PrecommitCallback &));
    MOCK_METHOD0(forceSend, void());
  };

}  // namespace fc::mining
