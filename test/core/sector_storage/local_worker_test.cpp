/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/local_worker.hpp"

#include <gtest/gtest.h>
#include "testutil/mocks/sector_storage/stores/local_store_mock.hpp"
#include "testutil/mocks/sector_storage/stores/sector_index_mock.hpp"
#include "testutil/mocks/sector_storage/stores/store_mock.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::primitives::WorkerConfig;
using fc::sector_storage::LocalWorker;
using fc::sector_storage::stores::LocalStoreMock;
using fc::sector_storage::stores::SectorIndexMock;
using fc::sector_storage::stores::StoreMock;

class LocalWorkerTest : public test::BaseFS_Test {
 public:
  LocalWorkerTest() : test::BaseFS_Test("fc_local_worker_test") {
    tasks_ = {
        fc::primitives::kTTAddPiece,
        fc::primitives::kTTPreCommit1,
        fc::primitives::kTTPreCommit2,
    };
    seal_proof_type_ = RegisteredProof::StackedDRG2KiBSeal;
    worker_name_ = "local worker";

    config_ = WorkerConfig{.hostname = worker_name_,
                           .seal_proof_type = seal_proof_type_,
                           .task_types = tasks_};
    store_ = std::make_shared<StoreMock>();
    local_store_ = std::make_shared<LocalStoreMock>();
    sector_index_ = std::make_shared<SectorIndexMock>();
    local_worker_ = std::make_unique<LocalWorker>(
        config_, store_, local_store_, sector_index_);
  }

 protected:
  std::vector<fc::primitives::TaskType> tasks_;
  RegisteredProof seal_proof_type_;
  std::string worker_name_;
  WorkerConfig config_;
  std::shared_ptr<StoreMock> store_;
  std::shared_ptr<LocalStoreMock> local_store_;
  std::shared_ptr<SectorIndexMock> sector_index_;
  std::unique_ptr<LocalWorker> local_worker_;
};
