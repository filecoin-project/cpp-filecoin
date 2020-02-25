/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/logger.hpp"
#include "graphsync_acceptance_common.hpp"

namespace fc::storage::ipfs::graphsync::test {


  void testTwoNodesExchange() {

  }

} // namespace fc::storage::ipfs::graphsync::test

TEST(GraphsyncAcceptance, TwoNodesExchange) {
  namespace test = fc::storage::ipfs::graphsync::test;

  test::testTwoNodesExchange( /*param*/ );
}

int main(int argc, char *argv[]) {
  if (argc > 1 && std::string("trace") == argv[1]) {
    auto log = fc::common::createLogger("graphsync");
    log->set_level(spdlog::level::trace);
    --argc;
    ++argv;
  } else {
    spdlog::set_level(spdlog::level::err);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
