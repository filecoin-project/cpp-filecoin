/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/storage/base_fs_test.hpp"

namespace test {

  BaseFS_Test::BaseFS_Test(fs::path path) : base_path(std::move(path)) {
    clear();
    mkdir();

    logger = fc::common::createLogger(getPathString());
    logger->set_level(spdlog::level::debug);
  }

  BaseFS_Test::~BaseFS_Test() {
    clear();
  }

  void BaseFS_Test::clear() {
    if (fs::exists(base_path)) {
      fs::remove_all(base_path);
    }
  }

  void BaseFS_Test::mkdir() {
    fs::create_directory(base_path);
  }

  std::string BaseFS_Test::getPathString() const {
    return fs::canonical(base_path).string();
  }

  fs::path BaseFS_Test::createDir(const fs::path &dirname) const {
    auto pathname =  base_path;
    pathname /= dirname;
    fs::create_directory(pathname);
    return pathname;
  }

  fs::path BaseFS_Test::createFile(const fs::path &filename) const {
    auto pathname =  base_path;
    pathname /= filename;
    boost::filesystem::ofstream ofs(pathname);
    ofs.close();
    return pathname;
  }

  bool BaseFS_Test::exists(const fs::path &entity) const {
    auto pathname =  base_path;
    pathname /= entity;
    return boost::filesystem::exists(pathname);
  }

  void BaseFS_Test::SetUp() {
    clear();
    mkdir();
  }

  void BaseFS_Test::TearDown() {
    clear();
  }
}  // namespace test
