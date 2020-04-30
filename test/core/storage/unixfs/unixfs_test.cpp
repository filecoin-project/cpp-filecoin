/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/unixfs/unixfs.hpp"

#include <gtest/gtest.h>

#include "common/span.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/outcome.hpp"

using Params = std::tuple<std::string, size_t, size_t, std::string>;
struct UnixfsTest : testing::TestWithParam<Params> {};

TEST_P(UnixfsTest, MatchGo) {
  auto &[data, chunk_size, max_links, cid_str] = GetParam();
  fc::storage::ipfs::InMemoryDatastore ipld;
  EXPECT_OUTCOME_TRUE(cid, fc::CID::fromString(cid_str));
  EXPECT_OUTCOME_EQ(
      fc::storage::unixfs::wrapFile(
          ipld, fc::common::span::cbytes(data), chunk_size, max_links),
      cid);
}

INSTANTIATE_TEST_CASE_P(
    UnixfsTestCases,
    UnixfsTest,
    ::testing::Values(
        Params{"[0     9)",
               10,
               2,
               "bafkreieqjdswwovkokzitxxsn4ppswzzsy2qs623duz5ivjhplglbygbou"},
        Params{"[0     10)",
               10,
               2,
               "bafkreiaymswyg525nktwb75n53rcwdqvjbfjgi5nu4glriglylthbnuxnu"},
        Params{"[0     10)[10   19)",
               10,
               2,
               "QmUyaoLFxpUpZra4qs65dMbrBMEERJt91JL3kdyKEXqLxN"},
        Params{"[0     10)[10     21)",
               10,
               2,
               "QmZ9KupPPphsds2Cbu7Ntb73tUHbyrZEgJisHMmw9AUfQP"},
        Params{"[0     10)[10    20)[10    30)",
               10,
               2,
               "QmSYvtzqeJY6zCRk31SdRweDD6z1bwP9GJY5bmKQ55XwWp"},
        Params{"[0     10)[10    20)[10     31)",
               10,
               2,
               "QmZ2Be9hJQhifpgASmQTGkL7j9KUaJC5W9siL6MnFSkLhd"},
        Params{"[0     10)[10    20)[10    30)",
               5,
               3,
               "QmYb4gZGCAkRdNZbc5npDzLJA34ZzPtdVDmESb9Xk76Ws2"}));
