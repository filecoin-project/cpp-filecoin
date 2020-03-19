/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/rpc/json.hpp"

#include <gtest/gtest.h>
#include <rapidjson/writer.h>

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::api::Address;
using fc::api::BigInt;
using fc::api::BlockHeader;
using fc::api::BlsSignature;
using fc::api::Buffer;
using fc::api::EPostProof;
using fc::api::EPostTicket;
using fc::api::MsgWait;
using fc::api::RleBitset;
using fc::api::Secp256k1Signature;
using fc::api::Signature;
using fc::api::Ticket;
using fc::api::TipsetKey;

#define J32 "\"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQE=\""
#define J96                                                                    \
  "\"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEB" \
  "AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEB\""

const auto b32 =
    "0101010101010101010101010101010101010101010101010101010101010101"_blob32;
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
  EXPECT_FALSE(document.HasParseError());
  return document;
}

template <typename T>
void expectJson(const T &value, std::string expected) {
  expected = jsonEncode(jsonDecode(expected));
  EXPECT_EQ(jsonEncode(fc::api::encode(value)), expected);
  EXPECT_OUTCOME_TRUE(decoded, fc::api::decode<T>(jsonDecode(expected)));
  EXPECT_EQ(jsonEncode(fc::api::encode(decoded)), expected);
}

/// Following tests check json encoding and decoding

TEST(ApiJsonTest, WrongType) {
  EXPECT_OUTCOME_ERROR(fc::api::JsonError::WRONG_TYPE,
                       fc::api::decode<Ticket>(jsonDecode("4")));
}

TEST(ApiJsonTest, Misc) {
  expectJson(INT64_C(-2), "-2");
  expectJson(std::vector<uint64_t>{1, 2}, "[1,2]");
  expectJson(RleBitset{2, 1}, "[1,2]");
  expectJson(boost::optional<uint64_t>{}, "null");
  expectJson(boost::make_optional(UINT64_C(2)), "2");
  expectJson(std::map<std::string, uint64_t>{{"a", 1}}, "{\"a\":1}");
  expectJson(std::make_tuple(UINT64_C(2), INT64_C(3)), "[2,3]");
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

TEST(ApiJsonTest, TipsetKey) {
  expectJson(TipsetKey{{"010001020001"_cid}}, "[{\"/\":\"baeaacaqaae\"}]");
}

TEST(ApiJsonTest, Address) {
  expectJson(Address::makeFromId(1), "\"t01\"");
  expectJson(Address::makeActorExec({}),
             "\"t2gncvesv7no7bqckesisllfzmif4qw3hs6fyf3iy\"");
}

TEST(ApiJsonTest, Signature) {
  expectJson(Signature{BlsSignature{b96}},
             "{\"Type\":\"bls\",\"Data\":" J96 "}");
  expectJson(Signature{Secp256k1Signature{"DEAD"_unhex}},
             "{\"Type\":\"secp256k1\",\"Data\":\"3q0=\"}");
}

TEST(ApiJsonTest, EPostProof) {
  expectJson(EPostProof{Buffer{"DEAD"_unhex}, b96, {EPostTicket{b32, 1, 2}}},
             "{\"Proof\":\"3q0=\",\"PostRand\":" J96
             ",\"Candidates\":[{\"Partial\":" J32
             ",\"SectorID\":1,\"ChallengeIndex\":2}]}");
}

TEST(ApiJsonTest, BigInt) {
  expectJson(BigInt{0}, "\"0\"");
  expectJson(BigInt{-1}, "\"-1\"");
  expectJson(BigInt{1}, "\"1\"");
}

TEST(ApiJsonTest, MsgWait) {
  expectJson(
      MsgWait{
          {1, Buffer{"DEAD"_unhex}, 2},
          {
              {"010001020001"_cid},
              {BlockHeader{
                  Address::makeFromId(1),
                  Ticket{b96},
                  {Buffer{"DEAD"_unhex}, b96, {}},
                  {"010001020002"_cid},
                  3,
                  4,
                  "010001020005"_cid,
                  "010001020006"_cid,
                  "010001020007"_cid,
                  "DEAD"_unhex,
                  8,
                  Signature{Secp256k1Signature{"DEAD"_unhex}},
                  9,
              }},
              3,
          },
      },
      "{\"Receipt\":{\"ExitCode\":1,\"Return\":\"3q0=\",\"GasUsed\":\"2\"},"
      "\"TipSet\":{\"Cids\":[{\"/"
      "\":\"baeaacaqaae\"}],\"Blocks\":[{\"Miner\":\"t01\",\"Ticket\":{"
      "\"VRFProof\":" J96
      "},\"EPostProof\":{\"Proof\":\"3q0=\",\"PostRand\":" J96
      ",\"Candidates\":[]},\"Parents\":[{\"/"
      "\":\"baeaacaqaai\"}],\"ParentWeight\":\"3\",\"Height\":4,"
      "\"ParentStateRoot\":{\"/"
      "\":\"baeaacaqaau\"},\"ParentMessageReceipts\":{\"/"
      "\":\"baeaacaqaay\"},\"Messages\":{\"/"
      "\":\"baeaacaqaa4\"},\"BLSAggregate\":{\"Type\":\"secp256k1\",\"Data\":"
      "\"3q0=\"},\"Timestamp\":8,\"BlockSig\":{\"Type\":\"secp256k1\",\"Data\":"
      "\"3q0=\"},\"ForkSignaling\":9}],\"Height\":3}}");
}
