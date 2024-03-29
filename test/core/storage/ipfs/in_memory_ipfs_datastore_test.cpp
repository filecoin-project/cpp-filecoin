/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

namespace fc::storage::ipfs {
  using libp2p::multi::HashType;
  using libp2p::multi::Multihash;

  class InMemoryIpfsDatastoreTest : public ::testing::Test {
   public:
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

    std::shared_ptr<IpfsDatastore> datastore{
        std::make_shared<InMemoryDatastore>()};
  };

  /**
   * @given opened datastore, cid and a value
   * @when put cid with value into datastore @and then get value from datastore
   * by cid
   * @then all operation succeeded, obtained value is equal to original value
   */
  TEST_F(InMemoryIpfsDatastoreTest, ContainsExistingTrueSuccess) {
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
    EXPECT_OUTCOME_EQ(datastore->contains(cid1), true);
  }

  /**
   * @given opened datastore, 2 different instances of CID and a value
   * @when put cid1 with value into datastore and check if datastore contains
   * cid2
   * @then all operations succeed and datastore doesn't contains cid2
   */
  TEST_F(InMemoryIpfsDatastoreTest, ContainsNotExistingFalseSuccess) {
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
    EXPECT_OUTCOME_EQ(datastore->contains(cid2), false);
  }

  /**
   * @given opened datastore, CID instance and a value
   * @when put cid with value into datastore @and then get value by cid
   * @then all operations succeed
   */
  TEST_F(InMemoryIpfsDatastoreTest, GetExistingSuccess) {
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
    EXPECT_OUTCOME_EQ(datastore->get(cid1), value);
  }

  /**
   * @given opened datastore, 2 different CID instances and a value
   * @when put cid1 with value into datastore @and then get value by cid2
   * @then put operation succeeds, get operation fails
   */
  TEST_F(InMemoryIpfsDatastoreTest, GetNotExistingFailure) {
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
    EXPECT_OUTCOME_ERROR(IpfsDatastoreError::kNotFound, datastore->get(cid2));
  }

  /** Setting same key twice succeeds */
  TEST_F(InMemoryIpfsDatastoreTest, SetTwice) {
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
    EXPECT_OUTCOME_TRUE_1(datastore->set(cid1, BytesIn{value}));
  }
}  // namespace fc::storage::ipfs
