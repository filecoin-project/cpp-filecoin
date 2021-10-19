/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/tarutil.hpp"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "common/span.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fs = boost::filesystem;

class TarUtilTest : public test::BaseFS_Test {
 public:
  TarUtilTest() : test::BaseFS_Test("fc_tar_util_test") {}
};

/**
 * @given tar file
 * @when try to extract it
 * @then all files are extracted. hierarchy and data do not changed.
 *
 * Tar file:
 * – Cache
 * – Seal
 * – Unseal
 *   |- test.txt
 */
TEST_F(TarUtilTest, extractTar) {
  EXPECT_OUTCOME_TRUE_1(
      fc::common::extractTar(resourcePath("sector.tar"), base_path));
  ASSERT_TRUE(fs::exists((base_path / "Cache").string()));
  ASSERT_TRUE(fs::exists((base_path / "Seal").string()));
  ASSERT_TRUE(fs::exists((base_path / "Unseal").string()));
  ASSERT_TRUE(fs::exists((base_path / "Unseal" / "test.txt").string()));
  auto data = readFile((base_path / "Unseal" / "test.txt").string());
  const std::string result = "some test data here\n";
  ASSERT_EQ(data, fc::common::span::cbytes(result));
}

/**
 * @given dir with dirs and file
 * @when try to zip it and extract
 * @then all files are extracted. hierarchy and data do not changed.
 *
 * Tar file:
 * - test
 * -- Empty
 * -- Cache
 *    |- test.txt
 */
TEST_F(TarUtilTest, zipTar) {
  auto root_path = base_path / "test";
  auto dir_path = root_path / "Cache";
  auto empty_dir_path = root_path / "Empty";
  auto file_path = dir_path / "test.txt";
  auto tar_path = base_path / "archive.tar";
  const std::string result_string = "Some test string\nfor check\nfunction\n";

  fs::create_directories(dir_path);
  fs::create_directory(empty_dir_path);
  std::ofstream input(file_path.string());
  input << result_string;
  input.close();

  EXPECT_OUTCOME_TRUE_1(fc::common::zipTar(root_path, tar_path));
  fs::remove_all(root_path);

  EXPECT_OUTCOME_TRUE_1(fc::common::extractTar(tar_path, base_path));
  ASSERT_TRUE(fs::exists(root_path) && fs::is_directory(root_path));
  ASSERT_TRUE(fs::exists(dir_path) && fs::is_directory(dir_path));
  ASSERT_TRUE(fs::exists(empty_dir_path) && fs::is_directory(empty_dir_path));
  ASSERT_TRUE(fs::exists(file_path) && fs::is_regular_file(file_path));

  ASSERT_EQ(std::distance(fs::directory_iterator(root_path),
                          fs::directory_iterator{}),
            2);
  ASSERT_EQ(
      std::distance(fs::directory_iterator(dir_path), fs::directory_iterator{}),
      1);
  ASSERT_EQ(std::distance(fs::directory_iterator(empty_dir_path),
                          fs::directory_iterator{}),
            0);

  ASSERT_EQ(readFile(file_path), fc::common::span::cbytes(result_string));
}
