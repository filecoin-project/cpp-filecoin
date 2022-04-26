/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "sector_storage/worker_estimator.hpp"

namespace fc::sector_storage {
  class EstimatorMock : public Estimator {
   public:
    MOCK_METHOD3(startWork, void(WorkerId, TaskType, CallId));

    MOCK_METHOD1(finishWork, void(CallId));
    MOCK_METHOD1(abortWork, void(CallId));

    MOCK_CONST_METHOD2(getTime, boost::optional<double>(WorkerId, TaskType));
  };
}  // namespace fc::sector_storage
