/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/store.hpp"

#include <gtest/gtest.h>
#include "sector_storage/stores/impl/local_store.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::sector_storage::stores::LocalStore;
using fc::sector_storage::stores::Store;

class LocalStoreTest : public test::BaseFS_Test {
 public:
  LocalStoreTest() : test::BaseFS_Test("fc_local_store_test") {
    local_store_ = std::make_shared<LocalStore>(
        nullptr, nullptr, gsl::make_span<std::string>(nullptr, 0));
  }

 protected:
  std::shared_ptr<Store> local_store_;
};
