/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/graphsync/extension.hpp"

#include <gtest/gtest.h>
#include "storage/ipfs/graphsync/impl/common.hpp"
#include "testutil/outcome.hpp"

using fc::CID;
using fc::storage::ipfs::graphsync::decodeDontSendCids;
using fc::storage::ipfs::graphsync::decodeResponseMetadata;
using fc::storage::ipfs::graphsync::encodeDontSendCids;
using fc::storage::ipfs::graphsync::encodeResponseMetadata;
using fc::storage::ipfs::graphsync::Error;
using fc::storage::ipfs::graphsync::Extension;
using fc::storage::ipfs::graphsync::ResponseMetadata;

/**
 * Test encode and decode ResponseMetadata extension
 */
TEST(GraphsyncExtensions, ResponseMetadataEncodeDecode) {
  ResponseMetadata response_metadata{};
  Extension extension = encodeResponseMetadata(response_metadata);
  EXPECT_OUTCOME_TRUE(decoded, decodeResponseMetadata(extension));
  EXPECT_EQ(decoded, response_metadata);
}

/**
 * @given wrong protocol name
 * @when try decode
 * @then error occured
 */
TEST(GraphsyncExtensions, ResponseMetadataWrongName) {
  Extension extension{.name = "wrong_name"};
  EXPECT_OUTCOME_ERROR(Error::MESSAGE_PARSE_ERROR,
                       decodeResponseMetadata(extension));
}

/**
 * Test encode and decode do-not-send-cids extension
 */
TEST(GraphsyncExtensions, DontSendCidsEncodeDecode) {
  std::vector<CID> cids;
  Extension extension = encodeDontSendCids(cids);
  EXPECT_OUTCOME_TRUE(decoded, decodeDontSendCids(extension));
  EXPECT_EQ(decoded, std::set<CID>(cids.begin(), cids.end()));
}

/**
 * @given wrong protocol name
 * @when try decode
 * @then error occured
 */
TEST(GraphsyncExtensions, DontSendCidsWrongName) {
  Extension extension{.name = "wrong_name"};
  EXPECT_OUTCOME_ERROR(Error::MESSAGE_PARSE_ERROR,
                       decodeDontSendCids(extension));
}
