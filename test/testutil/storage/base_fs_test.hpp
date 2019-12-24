/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_BASE_FS_TEST_HPP
#define CPP_FILECOIN_BASE_FS_TEST_HPP

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>

#include "common/logger.hpp"

// intentionally here, so users can use fs shortcut
namespace fs = boost::filesystem;

namespace test {

  /**
   * @brief Base test, which involves filesystem. Can be created with given
   * path. Clears path before test and after test.
   */
  struct BaseFS_Test : public ::testing::Test {
    // not explicit, intentionally
    explicit BaseFS_Test(const fs::path &path);

    ~BaseFS_Test() override;

    /**
     * @brief Delete directory and all containing files
     */
    void clear();

    /**
     * @brief Create testing directory
     */
    void mkdir();

    /**
     * @brief Get test directory path
     * @return path to test directory
     */
    std::string getPathString() const;

    /**
     * @brief Create subdirectory in test directory
     * @param dirname is a new subdirectory name
     * @return full pathname to the new subdirectory
     */
    fs::path createDir(const fs::path &dirname) const;

    /**
     * @brief create file in test directory
     * @param filename is a name of created file
     * @return full pathname to the new file
     */
    fs::path createFile(const fs::path &filename) const;

    /**
     * @brief path exists
     * @param entity - file or directory to check
     * @return
     */
    bool exists(const fs::path &entity) const;

    /**
     * @brief Create and clear directory before tests
     */
    void SetUp() override;

    /**
     * @brief Clear directory after tests
     */
    void TearDown() override;

   protected:
    fs::path base_path;
    fc::common::Logger logger;
  };

}  // namespace test

#endif  // CPP_FILECOIN_BASE_FS_TEST_HPP
