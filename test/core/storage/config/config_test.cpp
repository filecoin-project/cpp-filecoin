/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/config/config.hpp"

#include <gtest/gtest.h>

#include "testutil/storage/base_fs_test.hpp"

class ConfigImplTest : public test::BaseFS_Test {
 public:
  ConfigImplTest() : test::BaseFS_Test("/tmp/fc_config_test/") {}

  /**
   * Create a test directory with an empty file.
   */
  void SetUp() override {
    BaseFS_Test::SetUp();
  }
};

/**
 * @given
 * @when
 * @then
 */
TEST_F(ConfigImplTest, FileNotFound) {
  std::string cfg_file = fs::canonical(createFile("config.xml")).string();

  fc::storage::config::Config config{};
  config.Set("int", 1);
  config.Set("str", "string");
  config.Set("base.val", 231.21);
  config.Set("base.child.val", "text");
  config.Save(cfg_file);
  config.Load(cfg_file);
}
