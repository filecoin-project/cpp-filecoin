/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/cids_index/util.hpp"

#include <future>

#include "common/io_thread.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "storage/ipld/cids_ipld.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::storage::cids_index {
  using ipld::CidsIpld;

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

    void testFlush(std::shared_ptr<boost::asio::io_context> io);
  };

  TEST_F(CidsIndexTest, Flow) {
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

  TEST_F(CidsIndexTest, FlushOn) {
    ipld = *load(true);
    ipld->flush_on = 3;
    ipld->setCbor(1).value();
    EXPECT_EQ(ipld->written.size(), 1);
    ipld->setCbor(2).value();
    EXPECT_EQ(ipld->written.size(), 2);
    ipld->setCbor(3).value();
    EXPECT_EQ(ipld->written.size(), 0);
  }

  void CidsIndexTest::testFlush(std::shared_ptr<boost::asio::io_context> io) {
    ipld = *load(true);
    ipld->io = io;
    ipld->flush_on = 5;
    std::vector<CID> cids;
    for (auto i{0}; i < 40; ++i) {
      ipld->setCbor(i).value();
    }
    if (io) {
      std::promise<void> wait;
      io->post([&] { wait.set_value(); });
      wait.get_future().get();
    }
    for (auto i{0}; i < (int)cids.size(); ++i) {
      EXPECT_OUTCOME_EQ(ipld->getCbor<int>(cids[i]), i);
    }
  }

  TEST_F(CidsIndexTest, FlushSync) {
    testFlush(nullptr);
  }

  TEST_F(CidsIndexTest, FlushAsync) {
    IoThread thread;
    testFlush(thread.io);
  }
}  // namespace fc::storage::cids_index
