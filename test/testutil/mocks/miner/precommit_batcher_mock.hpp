/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/precommit_batcher.hpp"
#include <gmock/gmock.h>

namespace fc::mining{

  class PreCommitBatcherMock: public PreCommitBatcher{
   public:
   MOCK_METHOD4(addPreCommit, outcome::result<void> (
       const SectorInfo &,
       const TokenAmount &,
       const SectorPreCommitInfo &,
       const PrecommitCallback &));
   MOCK_METHOD0(forceSend, void());
  };



}
