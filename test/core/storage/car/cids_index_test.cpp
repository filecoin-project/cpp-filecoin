/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/cids_index/util.hpp"

#include "testutil/outcome.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::storage::cids_index {
  auto genesis_path{resourcePath("genesis.car")};
  auto value1{1};

  struct CidsIndexTest : test::BaseFS_Test {
    std::string car_path, cids_path;

    CidsIndexTest() : BaseFS_Test("cids_index_test") {
      car_path = (getPathString() / "test.car").string();
      cids_path = car_path + ".cids";
    }

    auto load(bool writable) {
      return loadOrCreateWithProgress(
          car_path, writable, boost::none, nullptr, nullptr);
    }
  };

  TEST_F(CidsIndexTest, Test) {
    std::shared_ptr<CidsIpld> ipld;

    // using existing car
    fs::copy(genesis_path, car_path);
    ipld = load(false).value();

    // must be writable
    EXPECT_OUTCOME_FALSE_1(ipld->setCbor(value1));
    ipld = load(true).value();
    EXPECT_OUTCOME_TRUE(cid1, ipld->setCbor(value1));
    EXPECT_OUTCOME_EQ(ipld->getCbor<decltype(value1)>(cid1), value1);

    // value persists
    ipld = load(true).value();
    EXPECT_OUTCOME_EQ(ipld->getCbor<decltype(value1)>(cid1), value1);

    // inserted only once
    auto car_size{fs::file_size(car_path)};
    EXPECT_OUTCOME_TRUE_1(ipld->setCbor(value1));
    ipld->car_file.flush();
    EXPECT_EQ(fs::file_size(car_path), car_size);
  }
}  // namespace fc::storage::cids_index
