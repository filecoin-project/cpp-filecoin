/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "serialization_vectors.hpp"

#include <fstream>

#include <gtest/gtest.h>
#include <rapidjson/writer.h>

#include "api/rpc/json.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::api::Codec;
using fc::api::UnsignedMessage;
using fc::common::hex_lower;

auto loadJson(const std::string &name) {
  std::ifstream file{
      std::filesystem::path{SERIALIZATION_VECTORS_ROOT}.append(name),
      std::ios::binary | std::ios::ate};
  EXPECT_TRUE(file.good());
  std::string str;
  str.resize(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(str.data(), str.size());
  rapidjson::Document document;
  document.Parse(str.data(), str.size());
  EXPECT_FALSE(document.HasParseError());
  return document;
}

TEST(SerializationVectorsTest, UnsignedMessage) {
  auto tests = loadJson("unsigned_messages.json");
  for (auto it = tests.Begin(); it != tests.End(); ++it) {
    auto message = Codec::decode<UnsignedMessage>(Codec::Get(*it, "message"));
    auto expected_cbor = Codec::AsString(Codec::Get(*it, "hex_cbor"));
    EXPECT_OUTCOME_TRUE(cbor, fc::codec::cbor::encode(message));
    EXPECT_EQ(hex_lower(cbor), expected_cbor);
  }
}
