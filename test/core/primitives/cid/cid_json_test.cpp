/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/primitives/cid/json_codec.hpp"

#include <gtest/gtest.h>
#include <boost/algorithm/string.hpp>
#include "filecoin/primitives/cid/cid_of_cbor.hpp"
#include "testutil/outcome.hpp"

using namespace fc::primitives::cid;

struct CidJsonTest : public ::testing::Test {
  void SetUp() override {
    EXPECT_OUTCOME_TRUE(c1, getCidOfCbor(std::string("string1")));
    EXPECT_OUTCOME_TRUE(c2, getCidOfCbor(std::string("string2")));
    EXPECT_OUTCOME_TRUE(c3, getCidOfCbor(std::string("string3")));

    cids.push_back(c1);
    cids.push_back(c2);
    cids.push_back(c3);
    // clang-format off
    json_value = "{\n"
                 "    \"\\/\": [\n"
                 "        \"GvZyPhweQo39L7iBjMEjnMq1SRRMZwTPHrXW6V8BZaFHPtngVTURUACfcXw\",\n"
                 "        \"GvZyPhweQo39L7iCp1hP7wV7i5dnfmVmsKzn1uSj1QCirrHLEiDs3emTnKc\",\n"
                 "        \"GvZyPhweQo39L7hysZK5AdeHeijLGMXWWGbpcWvnMW9XGqkRSeDrBZSBKAQ\"\n"
                 "    ]\n"
                 "}";
    // clang-format on
    stripString(json_value);
  }

  void stripString(std::string &s) {
    boost::erase_all(s, "\n");
    boost::erase_all(s, "\t");
    boost::erase_all(s, " ");
  }

  std::vector<fc::CID> cids;
  std::string json_value;
};

/**
 * @given vector of cids, predefined json value
 * @when json-encode cids using encodeCidVector function
 * @then resulting json is equal to predefined one
 * @when decode obtained json using decodeCidVector
 * @then resulting vector of cids is equal to the one given
 */
TEST_F(CidJsonTest, EncodeAndPrint) {
  EXPECT_OUTCOME_TRUE(value, fc::codec::json::encodeCidVector(cids));
  stripString(value);
  ASSERT_EQ(value, json_value);
  EXPECT_OUTCOME_TRUE(res, fc::codec::json::decodeCidVector(value));
  ASSERT_EQ(res, cids);
}
