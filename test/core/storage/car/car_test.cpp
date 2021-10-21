/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/car.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/filesystem/operations.hpp>

#include "codec/cbor/light_reader/block.hpp"
#include "primitives/block/block.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/ipld/memory_indexed_car.hpp"
#include "storage/unixfs/unixfs.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/resources.hpp"

namespace fc::storage::car {
  using ipfs::InMemoryDatastore;
  using primitives::ChainEpoch;
  namespace fs = boost::filesystem;

  /**
   * @given correct car file
   * @when loadCar
   * @then success
   */
  TEST(CarTest, LoadSuccess) {
    InMemoryDatastore ipld;
    EXPECT_OUTCOME_TRUE(input, common::readFile(resourcePath("genesis.car")));
    EXPECT_OUTCOME_TRUE(roots, loadCar(ipld, input));
    EXPECT_THAT(
        roots,
        testing::ElementsAre(
            "0171a0e402209a0640d0620af5d1c458effce4cbb8969779c9072b164d3fe6f5179d6378d8cd"_cid));
  }

  /**
   * @given correct car file
   * @when loadCar via path
   * @then success
   */
  TEST(CarTest, LoadFromFileSuccess) {
    InMemoryDatastore ipld;
    EXPECT_OUTCOME_TRUE(roots, loadCar(ipld, resourcePath("genesis.car")));
    EXPECT_THAT(
        roots,
        testing::ElementsAre(
            "0171a0e402209a0640d0620af5d1c458effce4cbb8969779c9072b164d3fe6f5179d6378d8cd"_cid));
  }

  /**
   * @given incorrect car file
   * @when loadCar
   * @then error
   */
  TEST(CarTest, LoadTruncatedError) {
    InMemoryDatastore ipld;
    EXPECT_OUTCOME_TRUE(input, common::readFile(resourcePath("genesis.car")));
    input.pop_back();
    EXPECT_OUTCOME_ERROR(CarError::kDecodeError, loadCar(ipld, input));
  }

  TEST(CarTest, MainnetGenesisBlockRead) {
    InMemoryDatastore ipld;
    EXPECT_OUTCOME_TRUE(roots, loadCar(ipld, resourcePath("genesis.car")));
    auto cbor{ipld.get(roots[0]).value()};
    BytesIn input{cbor};
    BytesIn ticket;
    BlockParentCbCids parents;
    ChainEpoch height;
    EXPECT_TRUE(
        codec::cbor::light_reader::readBlock(ticket, parents, height, input));
    EXPECT_EQ(height, 0);
    EXPECT_EQ(parents.size(), 0);
  }

  TEST(CarTest, MainnetGenesisBlockCbor) {
    InMemoryDatastore ipld;
    EXPECT_OUTCOME_TRUE(roots, loadCar(ipld, resourcePath("genesis.car")));
    auto cbor{ipld.get(roots[0]).value()};
    auto block{
        codec::cbor::decode<primitives::block::BlockHeader>(cbor).value()};
    EXPECT_TRUE(block.parents.empty());
    EXPECT_TRUE(block.parents.mainnet_genesis);
    EXPECT_OUTCOME_EQ(codec::cbor::encode(block), cbor);
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
    auto ipld1{std::make_shared<InMemoryDatastore>()};
    Sample2 obj2{2}, obj3{3};
    EXPECT_OUTCOME_TRUE(cid2, setCbor(ipld1, obj2));
    EXPECT_OUTCOME_TRUE(cid3, setCbor(ipld1, obj3));
    Sample1 obj1{{cid2}, {{"a", cid3}}};
    EXPECT_OUTCOME_TRUE(root, setCbor(ipld1, obj1));
    EXPECT_OUTCOME_TRUE(car, makeCar(*ipld1, {root}));

    InMemoryDatastore ipld2;
    EXPECT_OUTCOME_TRUE(roots, loadCar(ipld2, car));
    EXPECT_THAT(roots, testing::ElementsAre(root));
    EXPECT_OUTCOME_TRUE(raw1, ipld1->get(root));
    EXPECT_OUTCOME_EQ(ipld2.get(root), raw1);
    EXPECT_OUTCOME_TRUE(raw2, ipld1->get(cid2));
    EXPECT_OUTCOME_EQ(ipld2.get(cid2), raw2);
    EXPECT_OUTCOME_TRUE(raw3, ipld1->get(cid3));
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
    InMemoryDatastore ipld;
    EXPECT_OUTCOME_TRUE(input, common::readFile(PAYLOAD_FILE));
    EXPECT_OUTCOME_TRUE(root_cid, storage::unixfs::wrapFile(ipld, input));
    EXPECT_OUTCOME_EQ(
        root_cid.toString(),
        "bafk2bzaceccuidfq6dimuhvvnr5kb3zwjdobkob7bsuht4hdduawbx664suy4");
    EXPECT_OUTCOME_TRUE(selective_car,
                        makeSelectiveCar(ipld, {{root_cid, {}}}));

    EXPECT_OUTCOME_TRUE(expected_car, common::readFile(CAR_FROM_PAYLOAD_FILE));
    EXPECT_EQ(selective_car, expected_car)
        << std::endl
        << "actual" << std::endl
        << common::hex_upper(selective_car) << std::endl
        << "expected" << std::endl
        << common::hex_upper(expected_car) << std::endl;
  }

  /**
   * Interop test with go-fil-markets/storagemarket/integration_test.go
   * @given PAYLOAD_FILE with some data, cid_root of dag and selective_car bytes
   * from go implementation CAR_FROM_PAYLOAD_FILE
   * @when make selective_car file from PAYLOAD_FILE data and save it
   * @then selective_car file are equal to CAR_FROM_PAYLOAD_FILE
   */
  TEST(SelectiveCar, MakeSelectiveCarToFile) {
    InMemoryDatastore ipld;
    EXPECT_OUTCOME_TRUE(input, common::readFile(PAYLOAD_FILE));
    auto car_path = fs::temp_directory_path() / fs::unique_path();
    EXPECT_OUTCOME_TRUE(root_cid, storage::unixfs::wrapFile(ipld, input));
    EXPECT_OUTCOME_EQ(
        root_cid.toString(),
        "bafk2bzaceccuidfq6dimuhvvnr5kb3zwjdobkob7bsuht4hdduawbx664suy4");
    EXPECT_OUTCOME_TRUE_1(
        makeSelectiveCar(ipld, {{root_cid, {}}}, car_path.string()));

    EXPECT_OUTCOME_TRUE(expected_car, common::readFile(CAR_FROM_PAYLOAD_FILE));
    EXPECT_OUTCOME_TRUE(selective_car, common::readFile(car_path));
    EXPECT_EQ(selective_car, expected_car)
        << std::endl
        << "actual" << std::endl
        << common::hex_upper(selective_car) << std::endl
        << "expected" << std::endl
        << common::hex_upper(expected_car) << std::endl;
  }

  TEST(CarTest, MemoryIndexedCar) {
    InMemoryDatastore ipld;
    const auto path{resourcePath("genesis.car")};
    const auto roots{loadCar(ipld, path).value()};
    const auto mipld{MemoryIndexedCar::make(path.string(), false).value()};
    EXPECT_EQ(roots, mipld->roots);
    for (const auto &p : mipld->index) {
      const auto &cid{p.first};
      EXPECT_EQ(mipld->get(cid).value(), ipld.get(cid).value());
    }
  }
}  // namespace fc::storage::car
