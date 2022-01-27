/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "node/main/builder.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/resources.hpp"

namespace fc::node {

  /**
   * @when try load from non-existent file
   * @then error returned
   */
  TEST(Node, ReadWrongPrivateKey) {
    EXPECT_OUTCOME_FALSE_1(readPrivateKeyFromFile("wrong file"));
  }

  /**
   * @given file from lotus (see
   * https://github.com/filecoin-project/lotus/blob/master/cmd/lotus/daemon.go#L362-L394)
   * for loading procedure
   * @when file is loaded
   * @then file is bls and private key is equal to expected one
   */
  TEST(Node, ReadPrivateKey) {
    std::string path = resourcePath("node/lotus-key-import.key").string();
    EXPECT_OUTCOME_TRUE(key_info, readPrivateKeyFromFile(path));
    EXPECT_EQ(key_info.type, crypto::signature::Type::kBls);
    EXPECT_EQ(
        key_info.private_key,
        "8AD9F1D189F7602C8D776B3184642AA74B38CBA4B58B1232A397E2EB51A3B941"_unhex);
  }
}  // namespace fc::node
