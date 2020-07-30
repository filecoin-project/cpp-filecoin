/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_SELECTOR_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_SELECTOR_MOCK_HPP

#include "sector_storage/selector.hpp"

namespace fc::sector_storage {

  class SelectorMock : public WorkerSelector {
  public:
    MOCK_METHOD3(is_satisfying,
                 outcome::result<bool>(const TaskType &,
                                       RegisteredProof,
                                       const std::shared_ptr<WorkerHandle> &));

    MOCK_METHOD3(is_preferred,
                 outcome::result<bool>(const TaskType &,
                                       const std::shared_ptr<WorkerHandle> &,
                                       const std::shared_ptr<WorkerHandle> &));
  };

}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_SELECTOR_MOCK_HPP
