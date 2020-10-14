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
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/runtime/env.hpp"

const auto kCorpusRoot{resourcePath("test-vectors/corpus")};
auto brief(const std::string &path) {
  return path.substr(kCorpusRoot.size() + 1);
}

using fc::Buffer;
using fc::BytesIn;
using fc::CID;
using fc::IpldPtr;
using fc::primitives::BigInt;
using fc::primitives::ChainEpoch;
using fc::primitives::address::Address;
using fc::primitives::block::BlockHeader;
using fc::primitives::tipset::Tipset;
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
  struct Ts {
    struct Blk {
      Address miner;
      int64_t win_count;
      std::vector<UnsignedMessage> messages;
    };

    ChainEpoch epoch;
    BigInt base_fee;
    std::vector<Blk> blocks;
  };

  static auto decode(Json::JIn j) {
    using namespace Json;
    MessageVector mv;
    mv.type = *jStr(jGet(j, "class"));
    mv.car = gunzip(*jBytes(jGet(j, "car")));
    auto pre{jGet(j, "preconditions")}, post{jGet(j, "postconditions")};
    mv.state_before = *jCid(jGet(jGet(pre, "state_tree"), "root_cid"));
    mv.parent_epoch = *jInt(jGet(pre, "epoch"));
    mv.state_after = *jCid(jGet(jGet(post, "state_tree"), "root_cid"));
    if (auto messages{jGet(j, "apply_messages")}) {
      mv.messages = *jList(messages, [&](auto j) {
        return std::make_pair(
            *jInt(jGet(j, "epoch")),
            fc::codec::cbor::decode<UnsignedMessage>(*jBytes(jGet(j, "bytes")))
                .value());
      });
    }
    if (auto tipsets{jGet(j, "apply_tipsets")}) {
      mv.tipsets = *jList(tipsets, [](auto j) {
        return Ts{
            *jInt(jGet(j, "epoch")),
            *jInt(jGet(j, "basefee")),
            *jList(jGet(j, "blocks"),
                   [](auto j) {
                     return Ts::Blk{
                         fc::primitives::address::decodeFromString(
                             std::string{*jStr(jGet(j, "miner_addr"))})
                             .value(),
                         *jInt(jGet(j, "win_count")),
                         *jList(
                             jGet(j, "messages"),
                             [](auto j) {
                               return fc::codec::cbor::decode<UnsignedMessage>(
                                          *jBytes(j))
                                   .value();
                             }),
                     };
                   }),
        };
      });
    }
    mv.receipts = *jList(jGet(post, "receipts"), [](auto j) {
      return MessageReceipt{fc::vm::VMExitCode{*jInt(jGet(j, "exit_code"))},
                            *jBytes(jGet(j, "return")),
                            *jInt(jGet(j, "gas_used"))};
    });
    if (auto receipts_roots{jGet(post, "receipts_roots")}) {
      mv.receipts_roots =
          *jList(receipts_roots, [](auto j) { return *jCid(j); });
    }
    return mv;
  }

  std::string type;
  Buffer car;
  std::vector<Ts> tipsets;
  ChainEpoch parent_epoch;
  std::vector<std::pair<ChainEpoch, UnsignedMessage>> messages;
  std::vector<MessageReceipt> receipts;
  CID state_before, state_after;
  std::vector<CID> receipts_roots;
};

struct TestVectors : testing::TestWithParam<Param> {};

void testTipsets(const MessageVector &mv, IpldPtr ipld) {
  fc::vm::interpreter::InterpreterImpl vmi;
  CID state{mv.state_before};
  BlockHeader parent;
  parent.ticket.emplace();
  parent.height = mv.parent_epoch;
  parent.messages = parent.parent_message_receipts = parent.parent_state_root =
      mv.state_before;
  OUTCOME_EXCEPT(ipld->setCbor(parent));
  OUTCOME_EXCEPT(parents, Tipset::create({parent}));
  auto i{0}, j{0};
  for (auto &ts : mv.tipsets) {
    Tipset tipset;
    tipset.height = ts.epoch;
    for (auto &blk : ts.blocks) {
      auto &block{tipset.blks.emplace_back()};
      block.ticket.emplace();
      block.miner = blk.miner;
      block.election_proof.win_count = blk.win_count;
      block.height = ts.epoch;
      block.parents = parents.cids;
      block.parent_base_fee = ts.base_fee;
      fc::primitives::block::MsgMeta meta;
      ipld->load(meta);
      for (auto &msg : blk.messages) {
        if (msg.from.isBls()) {
          OUTCOME_EXCEPT(cid, ipld->setCbor(msg));
          OUTCOME_EXCEPT(meta.bls_messages.append(cid));
        } else {
          OUTCOME_EXCEPT(
              cid,
              ipld->setCbor(fc::vm::message::SignedMessage{
                  msg, fc::crypto::signature::Secp256k1Signature{}}));
          OUTCOME_EXCEPT(meta.secp_messages.append(cid));
        }
      }
      block.messages = ipld->setCbor(meta).value();
      block.parent_message_receipts = block.parent_state_root = state;
      OUTCOME_EXCEPT(cid, ipld->setCbor(block));
      tipset.cids.push_back(cid);
    }
    std::vector<MessageReceipt> receipts;
    OUTCOME_EXCEPT(res, vmi.interpret(ipld, tipset, &receipts));
    EXPECT_EQ(res.message_receipts, mv.receipts_roots[i]);
    for (auto &actual : receipts) {
      auto &expected{mv.receipts[j++]};
      EXPECT_EQ(actual.exit_code, expected.exit_code);
      EXPECT_EQ(actual.return_value, expected.return_value);
      EXPECT_EQ(actual.gas_used, expected.gas_used);
    }
    parents = tipset;
    ++i;
  }
  EXPECT_EQ(j, mv.receipts.size());
  EXPECT_EQ(state, mv.state_after);
}

void testMessages(const MessageVector &mv, IpldPtr ipld) {
  BlockHeader b;
  b.ticket.emplace();
  b.messages = b.parent_message_receipts = b.parent_state_root =
      mv.state_before;
  b.parent_base_fee = 100;
  OUTCOME_EXCEPT(ts, Tipset::create({b}));
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

TEST_P(TestVectors, Vector) {
  auto &path{GetParam()};
  fc::vm::actor::cgo::test_vectors = true;

  OUTCOME_EXCEPT(jdoc, Json::parse(*fc::common::readFile(path)));
  auto mv{MessageVector::decode(&jdoc)};
  auto ipld{std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  OUTCOME_EXCEPT(fc::storage::car::loadCar(*ipld, mv.car));

  if (mv.type == "tipset") {
    testTipsets(mv, ipld);
  } else if (mv.type == "message") {
    testMessages(mv, ipld);
  } else {
    FAIL();
  }
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
