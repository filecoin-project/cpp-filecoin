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
        Params{
            "[0     9)",
            10,
            2,
            "bafk2bzacebclln63l2eah6jlkgitda44alv3oog2mn3ge2e5vwisxygyvkv2c"},
        Params{
            "[0     10)",
            10,
            2,
            "bafk2bzacearlp5egyo7f5yeoqg2apmnhl2camb3rr435pjf4vmewbyytwbaak"},
        Params{"[0     10)[10   19)",
               10,
               2,
               "QmaUV8DCKX6MVg4sgifinKhQuFGNGJVZSTQ8msrMfCKu75"},
        Params{"[0     10)[10     21)",
               10,
               2,
               "Qmc8VppW9VWJkNYjL511i3E7vCoUpoQxr7uoo8HjwL2Equ"},
        Params{"[0     10)[10    20)[10    30)",
               10,
               2,
               "QmUZPND7FZUhNs2xWZGNBR8pTURTvaRR9NJg4RifJBcbMe"},
        Params{"[0     10)[10    20)[10     31)",
               10,
               2,
               "QmbwYUNQ15Y8CJFRJeT6wcAT2cNYoUDT15GkYQVgxAdLhd"},
        Params{"[0     10)[10    20)[10    30)",
               5,
               3,
               "QmTNpU9vuQaxroNTRpMvqdeSv2XNQNVoQrFiCxteZVdtTi"}));
