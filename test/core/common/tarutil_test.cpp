/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/tarutil.hpp"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "common/file.hpp"
#include "common/span.hpp"
#include "testutil/outcome.hpp"
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
      fc::common::extractTar(resourcePath("sector.tar"), base_path.string()));
  ASSERT_TRUE(fs::exists((base_path / "Cache").string()));
  ASSERT_TRUE(fs::exists((base_path / "Seal").string()));
  ASSERT_TRUE(fs::exists((base_path / "Unseal").string()));
  ASSERT_TRUE(fs::exists((base_path / "Unseal" / "test.txt").string()));
  std::ifstream input((base_path / "Unseal" / "test.txt").string());
  ASSERT_TRUE(input.good());
  const std::string result = "some test data here";
  std::string str;
  std::getline(input, str);
  ASSERT_EQ(str, result);
  input.close();
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

  EXPECT_OUTCOME_TRUE(fd, fc::common::zipTar(root_path.string()));
  auto _ = gsl::final_action([&]() { close(fd); });
  fs::remove_all(root_path);

  std::ofstream archive(tar_path.string(), std::ios_base::binary);

  char buff[8192];
  int len = read(fd, buff, sizeof(buff));
  while (len > 0) {
    archive.write(buff, len);
    len = read(fd, buff, sizeof(buff));
  }
  archive.close();

  EXPECT_OUTCOME_TRUE_1(
      fc::common::extractTar(tar_path.string(), base_path.string()));
  ASSERT_TRUE(fs::exists(root_path) && fs::is_directory(root_path));
  ASSERT_TRUE(fs::exists(dir_path) && fs::is_directory(root_path));
  ASSERT_TRUE(fs::exists(empty_dir_path) && fs::is_directory(root_path));
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

  EXPECT_OUTCOME_EQ(fc::common::readFile(file_path.string()),
                    fc::common::span::cbytes(gsl::make_span(
                        result_string.data(), result_string.size())));
}
