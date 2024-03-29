/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>

#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

namespace fc::storage::ipfs {
  using libp2p::multi::HashType;
  using libp2p::multi::Multihash;

  struct DatastoreIntegrationTest : public ::testing::Test {
    using Options = leveldb::Options;

    Options options{};

    CID cid1{CID::Version::V1,
             CID::Multicodec::SHA2_256,
             Multihash::create(HashType::sha256,
                               "0123456789ABCDEF0123456789ABCDEF"_unhex)
                 .value()};
    CID cid2{CID::Version::V1,
             CID::Multicodec::SHA2_256,
             Multihash::create(HashType::sha256,
                               "FEDCBA9876543210FEDCBA9876543210"_unhex)
                 .value()};

    Bytes value{"0123456789ABCDEF0123456789ABCDEF"_unhex};

    static boost::filesystem::path makeTempPath() {
      boost::filesystem::path global_temp_dir =
          boost::filesystem::temp_directory_path();
      return boost::filesystem::unique_path(
          global_temp_dir.append("%%%%%-%%%%%-%%%%%"));
    }

    void SetUp() override {
      leveldb_path = makeTempPath();
      options.create_if_missing = true;
      auto result = LeveldbDatastore::create(leveldb_path.string(), options);
      if (!result) boost::throw_exception(std::system_error(result.error()));
      datastore = result.value();
    }

    void TearDown() override {
      datastore.reset();
      if (boost::filesystem::exists(leveldb_path)) {
        boost::filesystem::remove_all(leveldb_path);
      }
    }

    boost::filesystem::path leveldb_path;
    std::shared_ptr<LeveldbDatastore> datastore;
  };

  /**
   * @given opened datastore, cid and a value
   * @when put cid with value into datastore @and then get value from datastore
   * by cid
   * @then all operation succeeded, obtained value is equal to original value
   */
  TEST_F(DatastoreIntegrationTest, ContainsExistingTrueSuccess) {
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
    EXPECT_OUTCOME_EQ(datastore->contains(cid1), true);
  }

  /**
   * @given opened datastore, 2 different instances of CID and a value
   * @when put cid1 with value into datastore and check if datastore contains
   * cid2
   * @then all operations succeed and datastore doesn't contains cid2
   */
  TEST_F(DatastoreIntegrationTest, ContainsNotExistingFalseSuccess) {
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
    EXPECT_OUTCOME_EQ(datastore->contains(cid2), false);
  }

  /**
   * @given opened datastore, CID instance and a value
   * @when put cid with value into datastore @and then get value by cid
   * @then all operations succeed
   */
  TEST_F(DatastoreIntegrationTest, GetExistingSuccess) {
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
    EXPECT_OUTCOME_EQ(datastore->get(cid1), value);
  }

  /**
   * @given opened datastore, 2 different CID instances and a value
   * @when put cid1 with value into datastore @and then get value by cid2
   * @then put operation succeeds, get operation fails
   */
  TEST_F(DatastoreIntegrationTest, GetNotExistingFailure) {
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
    EXPECT_OUTCOME_ERROR(IpfsDatastoreError::kNotFound, datastore->get(cid2));
  }

  /** Setting same key twice succeeds */
  TEST_F(DatastoreIntegrationTest, SetTwice) {
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
  }

  /**
   * @given opened datastore with some values stored
   * @when close datastore and open again
   * @then values are stored persistently
   */
  TEST_F(DatastoreIntegrationTest, PersistentStorage) {
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
    datastore.reset();

    EXPECT_OUTCOME_TRUE(
        open_again, LeveldbDatastore::create(leveldb_path.string(), options));
    EXPECT_OUTCOME_EQ(open_again->contains(cid1), true);
  }
}  // namespace fc::storage::ipfs
