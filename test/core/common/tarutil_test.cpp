/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/tarutil.hpp"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
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
