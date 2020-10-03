/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "storage/car/car.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/resources/resources.hpp"
#include "vm/actor/cgo/actors.hpp"
#include "vm/runtime/env.hpp"

const auto kCorpusRoot{resourcePath("test-vectors/corpus")};
auto brief(const std::string &path) {
  return path.substr(kCorpusRoot.size() + 1);
}

using fc::Buffer;
using fc::BytesIn;
using fc::CID;
using fc::primitives::ChainEpoch;
using fc::vm::message::UnsignedMessage;
using fc::vm::runtime::MessageReceipt;
using Param = std::string;
namespace Json = fc::codec::json;

auto gunzip(BytesIn input) {
  namespace bio = boost::iostreams;
  std::stringstream in{std::string{fc::common::span::bytestr(input)}}, out;
  bio::filtering_streambuf<bio::input> filter;
  filter.push(bio::gzip_decompressor());
  filter.push(in);
  bio::copy(filter, out);
  return Buffer{fc::common::span::cbytes(out.str())};
}

auto search() {
  std::vector<Param> paths;
  for (auto &item :
       boost::filesystem::recursive_directory_iterator{kCorpusRoot}) {
    auto &path{item.path()};
    if (item.status().type() == boost::filesystem::file_type::regular_file
        && path.extension() == ".json") {
      if (boost::algorithm::starts_with(path.filename().string(), "x--")) {
        continue;
      }
      paths.push_back(path.string());
    }
  }
  return paths;
}

struct MessageVector {
  static auto decode(Json::JIn j) {
    using namespace Json;
    MessageVector mv;
    EXPECT_EQ(*jStr(jGet(j, "class")), "message");
    mv.car = gunzip(*jBytes(jGet(j, "car")));
    auto pre{jGet(j, "preconditions")}, post{jGet(j, "postconditions")};
    mv.state_before = *jCid(jGet(jGet(pre, "state_tree"), "root_cid"));
    mv.state_after = *jCid(jGet(jGet(post, "state_tree"), "root_cid"));
    mv.messages = *jList(jGet(j, "apply_messages"), [&](auto j) {
      return std::make_pair(
          *jInt(jGet(j, "epoch")),
          fc::codec::cbor::decode<UnsignedMessage>(*jBytes(jGet(j, "bytes")))
              .value());
    });
    mv.receipts = *jList(jGet(post, "receipts"), [](auto j) {
      return MessageReceipt{fc::vm::VMExitCode{*jInt(jGet(j, "exit_code"))},
                            *jBytes(jGet(j, "return")),
                            *jInt(jGet(j, "gas_used"))};
    });
    return mv;
  }

  Buffer car;
  std::vector<std::pair<ChainEpoch, UnsignedMessage>> messages;
  std::vector<MessageReceipt> receipts;
  CID state_before, state_after;
};

struct TestVectors : testing::TestWithParam<Param> {};

TEST_P(TestVectors, Vector) {
  auto &path{GetParam()};
  fc::vm::actor::cgo::test_vectors = true;

  OUTCOME_EXCEPT(jdoc, Json::parse(*fc::common::readFile(path)));
  auto mv{MessageVector::decode(&jdoc)};
  auto ipld{std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  OUTCOME_EXCEPT(fc::storage::car::loadCar(*ipld, mv.car));

  fc::primitives::block::BlockHeader b;
  b.ticket.emplace();
  b.messages = b.parent_message_receipts = b.parent_state_root =
      mv.state_before;
  b.parent_base_fee = 100;
  OUTCOME_EXCEPT(ts, fc::primitives::tipset::Tipset::create({b}));
  auto env{std::make_shared<fc::vm::runtime::Env>(nullptr, ipld, ts)};
  auto i{0};
  for (auto &[epoch, message] : mv.messages) {
    auto &receipt{mv.receipts[i]};
    env->tipset.height = epoch;
    OUTCOME_EXCEPT(apply, env->applyMessage(message, message.chainSize()));
    EXPECT_EQ(apply.receipt.exit_code, receipt.exit_code);
    EXPECT_EQ(apply.receipt.return_value, receipt.return_value);
    EXPECT_EQ(apply.receipt.gas_used, receipt.gas_used);
    ++i;
  }
  OUTCOME_EXCEPT(state, env->state_tree->flush());
  EXPECT_EQ(state, mv.state_after);
}

INSTANTIATE_TEST_CASE_P(Vectors,
                        TestVectors,
                        testing::ValuesIn(search()),
                        [](auto &&p) {
                          auto s{brief(p.param)};
                          for (auto &c : s) {
                            if (!isalnum(c)) {
                              c = '_';
                            }
                          }
                          return s;
                        });
