/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/cids_index/util.hpp"

#include "primitives/cid/cid_of_cbor.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::storage::cids_index {
  template <typename T>
  auto makeValue(T v) {
    return std::make_pair(primitives::cid::getCidOfCbor(v).value(), v);
  }

  auto genesis_path{resourcePath("genesis.car")};
  auto [cid1, value1]{makeValue(1)};
  auto [cid2, value2]{makeValue(2)};

  struct CidsIndexTest : test::BaseFS_Test {
    std::string car_path, cids_path;
    std::shared_ptr<CidsIpld> ipld;

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
    // readable car must exist
    EXPECT_FALSE(fs::exists(car_path));
    EXPECT_OUTCOME_FALSE_1(load(false));

    // writeable car is created
    ipld = *load(true);
    EXPECT_TRUE(fs::exists(car_path));

    // readable car can't write
    ipld = *load(false);
    EXPECT_OUTCOME_FALSE_1(ipld->setCbor(value1));

    // writeable car can write
    ipld = *load(true);
    EXPECT_OUTCOME_TRUE_1(ipld->setCbor(value1));
    EXPECT_OUTCOME_EQ(ipld->getCbor<decltype(value1)>(cid1), value1);

    // value persists
    ipld = *load(true);
    EXPECT_OUTCOME_EQ(ipld->getCbor<decltype(value1)>(cid1), value1);

    // inserted only once
    auto car_value1{*common::readFile(car_path)};
    EXPECT_OUTCOME_TRUE_1(ipld->setCbor(value1));
    ipld->writable.flush();
    EXPECT_EQ(fs::file_size(car_path), car_value1.size());

    // truncated car drops index
    fs::resize_file(car_path, fs::file_size(car_path) - 1);
    ipld = *load(true);
    EXPECT_OUTCOME_EQ(ipld->contains(cid1), false);

    // changed car drops index
    EXPECT_OUTCOME_TRUE_1(ipld->setCbor(value2));
    ipld = *load(true);
    EXPECT_OUTCOME_EQ(ipld->getCbor<decltype(value2)>(cid2), value2);
    *common::writeFile(car_path, car_value1);
    ipld = *load(true);
    EXPECT_OUTCOME_EQ(ipld->contains(cid2), false);
    EXPECT_OUTCOME_EQ(ipld->getCbor<decltype(value1)>(cid1), value1);

    // incomplete car is truncated
    std::ofstream{car_path, std::ios::app | std::ios::binary} << "\x01";
    EXPECT_EQ(fs::file_size(car_path), car_value1.size() + 1);
    ipld = *load(true);
    EXPECT_OUTCOME_EQ(ipld->getCbor<decltype(value1)>(cid1), value1);
    EXPECT_EQ(fs::file_size(car_path), car_value1.size());

    // index is merged
    EXPECT_OUTCOME_TRUE_1(ipld->setCbor(value2));
    ipld = *load(true);
  }
}  // namespace fc::storage::cids_index
