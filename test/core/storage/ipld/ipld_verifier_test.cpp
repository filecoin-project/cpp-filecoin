/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipld/verifier.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

namespace fc::storage::ipld::verifier {
  using common::Buffer;
  using ipfs::IpfsDatastoreError;
  using traverser::TraverserError;

  struct ComplexIpldObject {
    std::vector<CID> list;
    std::map<std::string, CID> map;
  };
  CBOR_TUPLE(ComplexIpldObject, list, map);

  struct SimpleIpldObject {
    int i;
  };
  CBOR_TUPLE(SimpleIpldObject, i)

  class IpldVerifierTest : public ::testing::Test {
   public:
    SimpleIpldObject obj_2{2};
    Buffer data_2 = codec::cbor::encode(obj_2).value();
    CID cid_2 = common::getCidOf(data_2).value();

    SimpleIpldObject obj_3{3};
    Buffer data_3 = codec::cbor::encode(obj_3).value();
    CID cid_3 = common::getCidOf(data_3).value();

    ComplexIpldObject obj_1{{cid_2}, {{"a", cid_3}}};
    Buffer data_1 = codec::cbor::encode(obj_1).value();
    CID cid_1 = common::getCidOf(data_1).value();

    Verifier verifier{cid_1, {}};
  };

  /**
   * @given complex IPLD object consists of 3 blocks with cids
   * @when verifier is called with correct order
   * @then IPLD blocks successful verified
   */
  TEST_F(IpldVerifierTest, VerifySuccess) {
    EXPECT_OUTCOME_EQ(verifier.verifyNextBlock(cid_1, data_1), false);
    EXPECT_OUTCOME_EQ(verifier.verifyNextBlock(cid_2, data_2), false);
    EXPECT_OUTCOME_EQ(verifier.verifyNextBlock(cid_3, data_3), true);
  }

  /**
   * @given complex IPLD object consists of 3 blocks with cids
   * @when verifier is called with wrong cid
   * @then error returned
   */
  TEST_F(IpldVerifierTest, VerifyWrongCID) {
    EXPECT_OUTCOME_EQ(verifier.verifyNextBlock(cid_1, data_1), false);
    EXPECT_OUTCOME_ERROR(VerifierError::kUnexpectedCid,
                         verifier.verifyNextBlock(cid_2, data_3));
  }

  /**
   * @given complex IPLD object consists of 3 blocks with cids
   * @when verifier is called with wrong cid order
   * @then error returned
   */
  TEST_F(IpldVerifierTest, VerifyWrongOrder) {
    EXPECT_OUTCOME_EQ(verifier.verifyNextBlock(cid_1, data_1), false);
    EXPECT_OUTCOME_ERROR(IpfsDatastoreError::kNotFound,
                         verifier.verifyNextBlock(cid_3, data_3));
  }

  /**
   * @given complex IPLD object consists of 3 blocks with cids
   * @when verifier is called when traverse completed
   * @then error returned
   */
  TEST_F(IpldVerifierTest, VerifyExhausted) {
    EXPECT_OUTCOME_EQ(verifier.verifyNextBlock(cid_1, data_1), false);
    EXPECT_OUTCOME_EQ(verifier.verifyNextBlock(cid_2, data_2), false);
    EXPECT_OUTCOME_EQ(verifier.verifyNextBlock(cid_3, data_3), true);
    EXPECT_OUTCOME_ERROR(TraverserError::kTraverseCompleted,
                         verifier.verifyNextBlock(cid_3, data_3));
  }

  /**
   * @given complex IPLD object with duplicate blocks
   * @when verifier is called when traverse completed
   * @then error returned
   */
  TEST_F(IpldVerifierTest, VerifyComplexObkectWithDuplication) {
    ComplexIpldObject obj{{cid_2}, {{"a", cid_2}}};
    Buffer data = codec::cbor::encode(obj).value();
    CID root = common::getCidOf(data).value();
    Verifier verifier{root, {}};

    EXPECT_OUTCOME_EQ(verifier.verifyNextBlock(root, data), false);
    EXPECT_OUTCOME_EQ(verifier.verifyNextBlock(cid_2, data_2), false);
    EXPECT_OUTCOME_EQ(verifier.verifyNextBlock(cid_2, data_2), true);
  }

}  // namespace fc::storage::ipld::verifier
