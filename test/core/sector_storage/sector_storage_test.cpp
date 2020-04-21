/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

class SectorStorageTest : public test::BaseFS_Test {
 public:
  SectorStorageTest() : test::BaseFS_Test("fc_sector_storage_test") {}
};
