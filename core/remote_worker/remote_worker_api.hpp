/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/worker_api.hpp"
#include "sector_storage/impl/local_worker.hpp"

namespace fc::remote_worker {
  using fc::api::WorkerApi;
  using sector_storage::LocalWorker;
  using sector_storage::stores::LocalStore;

  std::shared_ptr<WorkerApi> makeWorkerApi(
      const std::shared_ptr<LocalStore> &local_store,
      const std::shared_ptr<LocalWorker> &worker);

}  // namespace fc::remote_worker