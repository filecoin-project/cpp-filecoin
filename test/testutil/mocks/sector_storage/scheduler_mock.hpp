/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "sector_storage/scheduler.hpp"

namespace fc::sector_storage {

  class SchedulerMock : public Scheduler {
   public:
    MOCK_METHOD7(schedule,
                 outcome::result<void>(const SectorRef &,
                                       const TaskType &,
                                       const std::shared_ptr<WorkerSelector> &,
                                       const WorkerAction &,
                                       const WorkerAction &,
                                       const ReturnCb &,
                                       uint64_t));

    MOCK_METHOD1(doNewWorker, void(WorkerHandle *));
    void newWorker(std::unique_ptr<WorkerHandle> worker) override {
      doNewWorker(worker.get());
    }

    MOCK_CONST_METHOD0(getSealProofType, RegisteredSealProof());

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

    MOCK_METHOD2(returnFetch,
                 outcome::result<void>(const CallId &,
                                       const boost::optional<CallError> &));

    MOCK_METHOD3(returnReadPiece,
                 outcome::result<void>(const CallId &,
                                       bool,
                                       const boost::optional<CallError> &));
  };

}  // namespace fc::sector_storage
