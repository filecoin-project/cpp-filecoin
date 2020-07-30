/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/car.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/unixfs/unixfs.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"

using fc::CID;
using fc::storage::car::CarError;
using fc::storage::car::loadCar;
using fc::storage::car::makeCar;
using fc::storage::car::makeSelectiveCar;
using fc::storage::ipfs::InMemoryDatastore;

/**
 * @given correct car file
 * @when loadCar
 * @then success
 */
TEST(CarTest, LoadSuccess) {
  InMemoryDatastore ipld;
  auto input = readFile(resourcePath("genesis.car"));
  EXPECT_OUTCOME_TRUE(roots, loadCar(ipld, input));
  EXPECT_THAT(
      roots,
      testing::ElementsAre(
          "0171a0e402202d4c00840d4227f198ec6c5343b8c70af7f008a7775d67393d10430ea2fa012f"_cid));
}

/**
 * @given incorrect car file
 * @when loadCar
 * @then error
 */
TEST(CarTest, LoadTruncatedError) {
  InMemoryDatastore ipld;
  auto input = readFile(resourcePath("genesis.car"));
  EXPECT_OUTCOME_ERROR(CarError::kDecodeError,
                       loadCar(ipld, input.subbuffer(0, input.size() - 1)));
}

struct Sample1 {
  std::vector<CID> list;
  std::map<std::string, CID> map;
};
CBOR_TUPLE(Sample1, list, map);

struct Sample2 {
  int i;
};
CBOR_TUPLE(Sample2, i)

TEST(CarTest, Writer) {
  InMemoryDatastore ipld1;
  Sample2 obj2{2}, obj3{3};
  EXPECT_OUTCOME_TRUE(cid2, ipld1.setCbor(obj2));
  EXPECT_OUTCOME_TRUE(cid3, ipld1.setCbor(obj3));
  Sample1 obj1{{cid2}, {{"a", cid3}}};
  EXPECT_OUTCOME_TRUE(root, ipld1.setCbor(obj1));
  EXPECT_OUTCOME_TRUE(car, makeCar(ipld1, {root}));

  InMemoryDatastore ipld2;
  EXPECT_OUTCOME_TRUE(roots, loadCar(ipld2, car));
  EXPECT_THAT(roots, testing::ElementsAre(root));
  EXPECT_OUTCOME_TRUE(raw1, ipld1.get(root));
  EXPECT_OUTCOME_EQ(ipld2.get(root), raw1);
  EXPECT_OUTCOME_TRUE(raw2, ipld1.get(cid2));
  EXPECT_OUTCOME_EQ(ipld2.get(cid2), raw2);
  EXPECT_OUTCOME_TRUE(raw3, ipld1.get(cid3));
  EXPECT_OUTCOME_EQ(ipld2.get(cid3), raw3);
}

/**
 * Interop test with go-fil-markets/storagemarket/integration_test.go
 * @given PAYLOAD_FILE with some data, cid_root of dag and selective_car bytes
 * from go implementation CAR_FROM_PAYLOAD_FILE
 * @when make selective_car file from PAYLOAD_FILE data
 * @then selective_car bytes are equal to CAR_FROM_PAYLOAD_FILE
 */
TEST(SelectiveCar, MakeSelectiveCar) {
  std::string root_cid_expected{
      "QmXFz92Uc9gCyAVGKkCzD84HEiR9fmrFzPSrvUypaN2Yzx"};

  InMemoryDatastore ipld;
  auto input = readFile(PAYLOAD_FILE);
  EXPECT_OUTCOME_TRUE(root_cid, fc::storage::unixfs::wrapFile(ipld, input));
  EXPECT_OUTCOME_EQ(root_cid.toString(), root_cid_expected);
  EXPECT_OUTCOME_TRUE(selective_car, makeSelectiveCar(ipld, {{root_cid, {}}}));

  auto expected_car = readFile(CAR_FROM_PAYLOAD_FILE);
  EXPECT_EQ(selective_car, expected_car) << std::endl
                                         << "actual" << std::endl
                                         << selective_car << std::endl
                                         << "expected" << std::endl
                                         << expected_car << std::endl;
}
