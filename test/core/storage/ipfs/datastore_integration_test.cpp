/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/impl/datastore_leveldb.hpp"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include "common/outcome_throw.hpp"
#include "testutil/outcome.hpp"

using fc::common::Buffer;
using fc::storage::LevelDB;
using fc::storage::ipfs::LeveldbDatastore;
using libp2p::crypto::random::BoostRandomGenerator;
using libp2p::crypto::random::CSPRNG;
using libp2p::multi::ContentIdentifier;
using libp2p::multi::HashType;
using libp2p::multi::MulticodecType;
using libp2p::multi::Multihash;

struct DatastoreIntegrationTest : public ::testing::Test {
  using Options = leveldb::Options;
  using CID = ContentIdentifier;

  static constexpr size_t kDefaultBufferLength = 32;

  static boost::filesystem::path makeTempPath() {
    boost::filesystem::path global_temp_dir =
        boost::filesystem::temp_directory_path();
    return boost::filesystem::unique_path(
        global_temp_dir.append("%%%%%-%%%%%-%%%%%"));
  }

  LeveldbDatastore::Value makeRandomBuffer(size_t size = kDefaultBufferLength) {
    return Buffer{generator->randomBytes(size)};
  }

  CID makeCid() {
    auto version = CID::Version::V1;
    auto codec_type = MulticodecType::SHA2_256;
    auto size = 32u;  // sha256 bytes count
    auto hash =
        Multihash::create(HashType::sha256, generator->randomBytes(size));
    if (!hash) boost::throw_exception(std::system_error(hash.error()));

    return ContentIdentifier(version, codec_type, hash.value());
  }

  void SetUp() override {
    leveldb_path = makeTempPath();
    Options options{};
    options.create_if_missing = true;
    auto result = LeveldbDatastore::create(leveldb_path.string(), options);
    if (!result) boost::throw_exception(std::system_error(result.error()));
    datastore = result.value();
    generator = std::make_shared<BoostRandomGenerator>();
  }

  void TearDown() override {
    datastore.reset();
    if (boost::filesystem::exists(leveldb_path)) {
      boost::filesystem::remove_all(leveldb_path);
    }
  }

  std::shared_ptr<CSPRNG> generator;
  boost::filesystem::path leveldb_path;
  std::shared_ptr<LeveldbDatastore> datastore;
};

/**
 * @given opened datastore, cid and a value
 * @when put cid with value into datastore @and then get value from datastore by
 * cid
 * @then all operation succeeded, obtained value is equal to original value
 */
TEST_F(DatastoreIntegrationTest, ContainsExistingTrueSuccess) {
  auto value = makeRandomBuffer();
  auto cid = makeCid();
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid, value));
  EXPECT_TRUE(datastore->contains(cid));
}

/**
 * @given opened datastore, 2 different instances of CID and a value
 * @when put cid1 with value into datastore and check if datastore contains cid2
 * @then all operations succeed and datastore doesn't contains cid2
 */
TEST_F(DatastoreIntegrationTest, ContainsNotExistingFalseSuccess) {
  auto cid1 = makeCid();
  auto cid2 = makeCid();
  auto value = makeRandomBuffer();
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, value));
  EXPECT_FALSE(datastore->contains(cid2));
}

/**
 * @given opened datastore, CID instance and a value
 * @when put cid with value into datastore @and then get value by cid
 * @then all operations succeed
 */
TEST_F(DatastoreIntegrationTest, GetExistingSuccess) {
  auto cid = makeCid();
  auto value = makeRandomBuffer();
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid, value));
  EXPECT_OUTCOME_TRUE(v, datastore->get(cid));
  ASSERT_EQ(v, value);
}

/**
 * @given opened datastore, 2 different CID instances and a value
 * @when put cid1 with value into datastore @and then get value by cid2
 * @then put operation succeeds, get operation fails
 */
TEST_F(DatastoreIntegrationTest, GetNotExistingFailure) {
  auto cid1 = makeCid();
  auto cid2 = makeCid();
  auto value = makeRandomBuffer();
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, value));
  EXPECT_OUTCOME_FALSE_1(datastore->get(cid2));
}

/**
 * @given opened datastore, CID instance and a value
 * @when put cid with value into datastore @and remove cid from datastore
 * @then all operations succeed and datastore doesn't contain cid anymore
 */
TEST_F(DatastoreIntegrationTest, RemoveSuccess) {
  auto cid = makeCid();
  auto value = makeRandomBuffer();
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid, value));
  EXPECT_OUTCOME_TRUE_1(datastore->remove(cid));
  ASSERT_FALSE(datastore->contains(cid));
}

/**
 * @given opened datastore, 2 CID instances and a value
 * @when put cid1 with value into datastore @and remove cid2 from datastore
 * @then all operations succeed and datastore still contains cid1
 */
TEST_F(DatastoreIntegrationTest, RemoveNotExistingSuccess) {
  auto cid1 = makeCid();
  auto cid2 = makeCid();
  auto value = makeRandomBuffer();
  EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, value));
  EXPECT_OUTCOME_TRUE_1(datastore->remove(cid2));
  ASSERT_TRUE(datastore->contains(cid1));
}
