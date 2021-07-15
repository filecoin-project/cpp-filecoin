/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "sector_storage/worker.hpp"

namespace fc::sector_storage {

  class WorkerReturnMock : public WorkerReturn {
   public:
    MOCK_METHOD3(returnAddPiece,
                 outcome::result<void>(const CallId &,
                                       const PieceInfo &,
                                       const boost::optional<CallError> &));

    MOCK_METHOD3(returnSealPreCommit1,
                 outcome::result<void>(const CallId &,
                                       const PreCommit1Output &,
                                       const boost::optional<CallError> &));

    MOCK_METHOD3(returnSealPreCommit2,
                 outcome::result<void>(const CallId &,
                                       const SectorCids &,
                                       const boost::optional<CallError> &));

    MOCK_METHOD3(returnSealCommit1,
                 outcome::result<void>(const CallId &,
                                       const Commit1Output &,
                                       const boost::optional<CallError> &));

    MOCK_METHOD3(returnSealCommit2,
                 outcome::result<void>(const CallId &,
                                       const Proof &,
                                       const boost::optional<CallError> &));

    MOCK_METHOD2(returnFinalizeSector,
                 outcome::result<void>(const CallId &,
                                       const boost::optional<CallError> &));

    MOCK_METHOD2(returnMoveStorage,
                 outcome::result<void>(const CallId &,
                                       const boost::optional<CallError> &));

    MOCK_METHOD2(returnUnsealPiece,
                 outcome::result<void>(const CallId &,
                                       const boost::optional<CallError> &));

    MOCK_METHOD3(returnReadPiece,
                 outcome::result<void>(const CallId &,
                                       bool,
                                       const boost::optional<CallError> &));

    MOCK_METHOD2(returnFetch,
                 outcome::result<void>(const CallId &,
                                       const boost::optional<CallError> &));
  };

}  // namespace fc::sector_storage
