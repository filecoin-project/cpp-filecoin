/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/storage_impl.hpp"

#include <gtest/gtest.h>
#include <sector_storage/stores/storage_error.hpp>

#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "testutil/default_print.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::sector_storage::stores {
  class LocalStorageTest : public test::BaseFS_Test {
   public:
    LocalStorageTest() : test::BaseFS_Test("fc_local_storage_test") {
      storage_ = std::make_unique<LocalStorageImpl>(base_path.string());
      storage_config_ = StorageConfig{
          .storage_paths = {LocalPath{.path = (base_path / "some1").string()}},
      };
    }

   protected:
    std::shared_ptr<LocalStorage> storage_;
    StorageConfig storage_config_;
  };

  /**
   * @given empty storage
   * @when try to get storage config
   * @then returns none
   */
  TEST_F(LocalStorageTest, GetStorageFileNotExist) {
    EXPECT_OUTCOME_EQ(storage_->getStorage(), boost::none);
  }

  /**
   * @given storage with the config
   * @when try to get storage config
   * @then the config is returned
   */
  TEST_F(LocalStorageTest, GetStorage) {
    EXPECT_OUTCOME_TRUE(text,
                        codec::json::format(api::encode(storage_config_)));
    OUTCOME_EXCEPT(common::writeFile(base_path / kStorageConfig, text));
    EXPECT_OUTCOME_EQ(storage_->getStorage(), storage_config_);
  }

  /**
   * @given storage with the config
   * @when try to apply setStorage and get new config
   * @then new config is returned
   */
  TEST_F(LocalStorageTest, SetStorage) {
    EXPECT_OUTCOME_TRUE(text,
                        codec::json::format(api::encode(storage_config_)));
    OUTCOME_EXCEPT(common::writeFile(base_path / kStorageConfig, text));
    auto new_path = (base_path / "some2").string();
    EXPECT_OUTCOME_TRUE_1(storage_->setStorage([&](StorageConfig &config) {
      config.storage_paths.push_back(LocalPath{new_path});
    }));
    storage_config_.storage_paths.push_back(LocalPath{new_path});

    EXPECT_OUTCOME_EQ(storage_->getStorage(), storage_config_);
  }
}  // namespace fc::sector_storage::stores
