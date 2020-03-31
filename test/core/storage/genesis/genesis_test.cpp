/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/genesis/genesis.hpp"
#include "genesis_test.hpp"

#include <fstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::common::Buffer;
using fc::storage::genesis::CarError;
using fc::storage::genesis::loadCar;
using fc::storage::ipfs::InMemoryDatastore;

auto readFile(std::string_view path) {
  std::ifstream file{path, std::ios::binary | std::ios::ate};
  EXPECT_TRUE(file.good());
  Buffer buffer;
  buffer.resize(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
  return buffer;
}

TEST(GenesisTest, LoadSuccess) {
  InMemoryDatastore ipld;
  auto input = readFile(GENESIS_FILE);
  EXPECT_OUTCOME_TRUE(roots, loadCar(ipld, input));
  EXPECT_THAT(
      roots,
      testing::ElementsAre(
          "0171a0e402202ecd6c8f4c987ff715c888294420aad8a15db507bc150c81189b8b6c2988bfca"_cid));
}

TEST(GenesisTest, LoadTruncatedError) {
  InMemoryDatastore ipld;
  auto input = readFile(GENESIS_FILE);
  EXPECT_OUTCOME_ERROR(CarError::DECODE_ERROR,
                       loadCar(ipld, input.subbuffer(0, input.size() - 1)));
}
