/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/car/car.hpp"
#include "car_test.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"

using fc::storage::car::CarError;
using fc::storage::car::loadCar;
using fc::storage::ipfs::InMemoryDatastore;

/**
 * @given correct car file
 * @when loadCar
 * @then success
 */
TEST(GenesisTest, LoadSuccess) {
  InMemoryDatastore ipld;
  auto input = readFile(GENESIS_FILE);
  EXPECT_OUTCOME_TRUE(roots, loadCar(ipld, input));
  EXPECT_THAT(
      roots,
      testing::ElementsAre(
          "0171a0e402202ecd6c8f4c987ff715c888294420aad8a15db507bc150c81189b8b6c2988bfca"_cid));
}

/**
 * @given incorrect car file
 * @when loadCar
 * @then error
 */
TEST(GenesisTest, LoadTruncatedError) {
  InMemoryDatastore ipld;
  auto input = readFile(GENESIS_FILE);
  EXPECT_OUTCOME_ERROR(CarError::DECODE_ERROR,
                       loadCar(ipld, input.subbuffer(0, input.size() - 1)));
}
