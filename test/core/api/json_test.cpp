/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/json.hpp"

#include <gtest/gtest.h>
#include <rapidjson/writer.h>

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::api::Ticket;

#define J96                                                                    \
  "\"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEB" \
  "AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEB\""

const auto b96 =
    "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;

auto jsonEncode(const rapidjson::Value &value) {
  rapidjson::StringBuffer buffer;
  auto writer = rapidjson::Writer<rapidjson::StringBuffer>{buffer};
  value.Accept(writer);
  return std::string{buffer.GetString(), buffer.GetSize()};
}

auto jsonDecode(const std::string &str) {
  rapidjson::Document document;
  document.Parse(str.data(), str.size());
  return document;
}

template <typename T>
void expectJson(const T &value, std::string expected) {
  expected = jsonEncode(jsonDecode(expected));
  EXPECT_EQ(jsonEncode(fc::api::encode(value)), expected);
  EXPECT_OUTCOME_TRUE(decoded, fc::api::decode<T>(jsonDecode(expected)));
  EXPECT_EQ(jsonEncode(fc::api::encode(decoded)), expected);
}

TEST(ApiJsonTest, WrongType) {
  EXPECT_OUTCOME_ERROR(fc::api::JsonError::WRONG_TYPE,
                       fc::api::decode<Ticket>(jsonDecode("4")));
}

TEST(ApiJsonTest, CID) {
  expectJson("010001020001"_cid, "{\"/\":\"baeaacaqaae\"}");
  expectJson(
      "122059ca84fb79f2a7447b9e82c7412df58c688910cba202b7d4e9bf329ce07f931c"_cid,
      "{\"/\":\"QmUPA6yhRBJdB6XZrXE756qBzCiEq4QXHRVX5m5Rd4Jq9u\"}");
}

TEST(ApiJsonTest, Ticket) {
  expectJson(Ticket{b96}, "{\"VRFProof\":" J96 "}");
}
