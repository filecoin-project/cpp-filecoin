/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/assert.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include "codec/json/json.hpp"
#include "core/test_vectors/replaying_randomness.hpp"
#include "primitives/tipset/load.hpp"
#include "proofs/proof_param_provider.hpp"
#include "storage/car/car.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"
#include "vm/actor/cgo/actors.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/runtime/env.hpp"

// TODO (a.chernyshov) add chaos actor and enable
/** Enable tests with chaos actor. */
const bool kEnableChaos = false;

const auto kCorpusRoot{resourcePath("test-vectors/corpus")};
inline auto brief(const std::string &path) {
  auto n{kCorpusRoot.string().size() + 1};
  return path.substr(n, path.size() - n - 5);
}

inline auto testName(const boost::filesystem::path &path) {
  auto s = brief(path.string());
  for (auto &c : s) {
    if (!isalnum(c)) {
      c = '_';
    }
  }
  return s;
}

using fc::Bytes;
using fc::BytesIn;
using fc::CID;
using fc::IpldPtr;
using fc::crypto::randomness::DomainSeparationTag;
using fc::crypto::randomness::Randomness;
using fc::primitives::BigInt;
using fc::primitives::ChainEpoch;
using fc::primitives::EpochDuration;
using fc::primitives::address::Address;
using fc::primitives::block::BlockHeader;
using fc::primitives::tipset::put;
using fc::primitives::tipset::Tipset;
using fc::vm::actor::Invoker;
using fc::vm::actor::InvokerImpl;
using fc::vm::message::UnsignedMessage;
using fc::vm::runtime::FixedRandomness;
using fc::vm::runtime::MessageReceipt;
using fc::vm::runtime::RandomnessType;
using fc::vm::runtime::ReplayingRandomness;
using fc::vm::runtime::RuntimeRandomness;
using fc::vm::runtime::TestVectorRandomness;
namespace Json = fc::codec::json;

auto gunzip(BytesIn input) {
  namespace bio = boost::iostreams;
  std::stringstream in{std::string{fc::common::span::bytestr(input)}}, out;
  bio::filtering_streambuf<bio::input> filter;
  filter.push(bio::gzip_decompressor());
  filter.push(in);
  bio::copy(filter, out);
  return fc::copy(fc::common::span::cbytes(out.str()));
}

struct MessageVector {
  /**
   * Execution preconditions that need to be applied and satisfied before this
   * test vector can be executed
   */
  struct PreconditionVariant {
    // codename of the protocol version
    std::string id;
    // epoch at which to run
    ChainEpoch epoch;
    // network version with which to run
    unsigned int network_version;
  };

  struct Ts {
    struct Blk {
      Address miner;
      int64_t win_count{};
      std::vector<UnsignedMessage> messages;
    };

    EpochDuration epoch_offset;
    BigInt base_fee;
    std::vector<Blk> blocks;
  };

  static auto decode(Json::JIn j) {
    using namespace Json;
    MessageVector mv;
    mv.type = *jStr(*jGet(j, "class"));
    mv.car = gunzip(*jBytes(*jGet(j, "car")));

    if (auto randomness{jGet(j, "randomness")}) {
      mv.randomness = *jList(*randomness, [](auto j) {
        TestVectorRandomness new_randomness;
        // "on" in randomness is an array
        auto it = j->FindMember("on")->value.Begin();
        // 1st element is randomness type in string
        auto randomness_type = *jStr(it);
        if (randomness_type == "chain") {
          new_randomness.type = RandomnessType::kChain;
        } else if (randomness_type == "beacon") {
          new_randomness.type = RandomnessType::kBeacon;
        } else {
          BOOST_ASSERT_MSG(false, "Wrong randomness type");
        }
        // 2nd element is domain separation tag
        new_randomness.domain_separation_tag =
            DomainSeparationTag{*jUint(++it)};
        // 3rd element is epoch
        new_randomness.epoch = *jInt(++it);
        // 4th element is entropy
        new_randomness.entropy = *jBytes(++it);

        auto ret = jBytes(*jGet(j, "ret")).value();
        BOOST_ASSERT_MSG(ret.size() == new_randomness.ret.size(),
                         "Wrong randomness size");
        std::move(
            ret.begin(), ret.begin() + ret.size(), new_randomness.ret.begin());
        return new_randomness;
      });
    }

    auto pre{*jGet(j, "preconditions")};
    mv.state_before = *jCid(*jGet(*jGet(pre, "state_tree"), "root_cid"));
    mv.precondition_variants = *jList(*jGet(pre, "variants"), [&](auto j) {
      PreconditionVariant precondition_variant;
      precondition_variant.id = *jStr(*jGet(j, "id"));
      precondition_variant.epoch = *jInt(*jGet(j, "epoch"));
      precondition_variant.network_version = *jInt(*jGet(j, "nv"));
      return precondition_variant;
    });

    if (auto base_fee{jGet(pre, "basefee")}) {
      mv.parent_base_fee = *jBigInt(*base_fee);
    } else {
      mv.parent_base_fee = 100;
    }
    auto post{*jGet(j, "postconditions")};
    mv.state_after = *jCid(*jGet(*jGet(post, "state_tree"), "root_cid"));
    if (auto selector{jGet(j, "selector")}) {
      if (auto chaos{jGet(*selector, "chaos_actor")}) {
        mv.chaos = *jStr(*chaos) == "true";
      }
    }
    if (auto messages{jGet(j, "apply_messages")}) {
      mv.messages = *jList(*messages, [&](auto j) {
        ChainEpoch epoch_offset{};
        if (auto maybe_epoch_offset{jGet(j, "epoch_offset")}) {
          epoch_offset = *jInt(*maybe_epoch_offset);
        }
        return std::make_pair(
            epoch_offset,
            fc::codec::cbor::decode<UnsignedMessage>(*jBytes(*jGet(j, "bytes")))
                .value());
      });
    }
    if (auto tipsets{jGet(j, "apply_tipsets")}) {
      mv.tipsets = *jList(*tipsets, [](auto j) {
        return Ts{
            *jInt(*jGet(j, "epoch_offset")),
            *jInt(*jGet(j, "basefee")),
            *jList(*jGet(j, "blocks"),
                   [](auto j) {
                     return Ts::Blk{
                         fc::primitives::address::decodeFromString(
                             std::string{*jStr(*jGet(j, "miner_addr"))})
                             .value(),
                         *jInt(*jGet(j, "win_count")),
                         *jList(
                             *jGet(j, "messages"),
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
    mv.receipts = *jList(*jGet(post, "receipts"), [](auto j) {
      return MessageReceipt{fc::vm::VMExitCode{*jInt(*jGet(j, "exit_code"))},
                            *jBytes(*jGet(j, "return")),
                            *jInt(*jGet(j, "gas_used"))};
    });
    if (auto receipts_roots{jGet(post, "receipts_roots")}) {
      mv.receipts_roots =
          *jList(*receipts_roots, [](auto j) { return *jCid(j); });
    }
    return mv;
  }

  static auto read(const boost::filesystem::path &path) {
    auto jdoc{*Json::parse(readFile(path))};
    auto mv{MessageVector::decode(&jdoc)};
    mv.path = path;
    return mv;
  }

  std::string type;
  Bytes car;
  std::vector<Ts> tipsets;
  std::vector<TestVectorRandomness> randomness;
  std::vector<PreconditionVariant> precondition_variants;
  BigInt parent_base_fee;
  // chain epoch offset and message
  std::vector<std::pair<EpochDuration, UnsignedMessage>> messages;
  std::vector<MessageReceipt> receipts;
  CID state_before, state_after;
  std::vector<CID> receipts_roots;
  bool chaos{false};
  boost::filesystem::path path;
};

/**
 * Search kCorpusRoot ("resources/test-vectors/corpus") for test vector files
 * @param chaos_enabled - do not skip tests with chaos actor
 * @return vector of tests
 */
auto search() {
  std::vector<MessageVector> vectors;

  for (auto &item :
       boost::filesystem::recursive_directory_iterator{kCorpusRoot}) {
    const auto &path{item.path()};
    if (item.status().type() == boost::filesystem::file_type::regular_file
        && path.extension() == ".json") {
      // ignore broken/incorrect vectors that starts with "x--"
      if (boost::algorithm::starts_with(path.filename().string(), "x--")) {
        continue;
      }

      auto vector = MessageVector::read(path);
      // skip tests with chaos actors
      if (!kEnableChaos && vector.chaos) {
        continue;
      }
      vectors.push_back(vector);
    }
  }
  return vectors;
}

struct TestVectors : testing::TestWithParam<MessageVector> {
  static void SetUpTestCase() {
    // Download proofs needed for tests
    OUTCOME_EXCEPT(params,
                   fc::proofs::ProofParamProvider::readJson(
                       "/var/tmp/filecoin-proof-parameters/parameters.json"));
    fc::primitives::sector::RegisteredSealProof seal_proof_type =
        fc::primitives::sector::RegisteredSealProof::kStackedDrg2KiBV1;
    OUTCOME_EXCEPT(sector_size,
                   fc::primitives::sector::getSectorSize(seal_proof_type));
    OUTCOME_EXCEPT(
        fc::proofs::ProofParamProvider::getParams(params, sector_size));
  }
};

void testTipsets(const MessageVector &mv, const IpldPtr &ipld) {
  using namespace fc;
  for (const auto &precondition : mv.precondition_variants) {
    std::shared_ptr<Invoker> invoker = std::make_shared<InvokerImpl>();
    std::shared_ptr<RuntimeRandomness> randomness =
        std::make_shared<ReplayingRandomness>(mv.randomness);
    auto ts_load{std::make_shared<fc::primitives::tipset::TsLoadIpld>(ipld)};
    fc::vm::runtime::EnvironmentContext env_context{
        ipld, invoker, randomness, ts_load};
    fc::vm::interpreter::InterpreterImpl vmi{env_context, nullptr};
    CID state{mv.state_before};
    BlockHeader parent;
    parent.ticket.emplace();
    parent.height = precondition.epoch;
    parent.messages = parent.parent_message_receipts =
        parent.parent_state_root = mv.state_before;
    put(ipld, nullptr, parent);
    OUTCOME_EXCEPT(parents, Tipset::create({parent}));
    auto i{0};
    auto j{0};
    for (const auto &ts : mv.tipsets) {
      fc::primitives::tipset::TipsetCreator cr;
      fc::primitives::block::Ticket ticket{{0}};
      for (const auto &blk : ts.blocks) {
        fc::primitives::block::BlockHeader block;
        block.ticket.emplace(ticket);
        ++ticket.bytes[0];
        block.miner = blk.miner;
        block.election_proof.win_count = blk.win_count;
        block.height = precondition.epoch + ts.epoch_offset;
        (std::vector<fc::CbCid> &)block.parents = parents->key.cids();
        block.parent_base_fee = ts.base_fee;
        fc::primitives::block::MsgMeta meta;
        cbor_blake::cbLoadT(ipld, meta);
        for (const auto &msg : blk.messages) {
          OUTCOME_EXCEPT(unsigned_cid, setCbor(ipld, msg));
          OUTCOME_EXCEPT(
              signed_cid,
              setCbor(ipld,
                      fc::vm::message::SignedMessage{
                          msg, fc::crypto::signature::Secp256k1Signature{}}));
          if (msg.from.isBls()) {
            OUTCOME_EXCEPT(meta.bls_messages.append(unsigned_cid));
          } else if (msg.from.isSecp256k1()) {
            OUTCOME_EXCEPT(meta.secp_messages.append(signed_cid));
          } else {
            // sneak in messages originating from other addresses as both kinds.
            // these should fail, as they are actually invalid senders.
            OUTCOME_EXCEPT(meta.bls_messages.append(unsigned_cid));
            OUTCOME_EXCEPT(meta.secp_messages.append(signed_cid));
          }
        }
        block.messages = setCbor(ipld, meta).value();
        block.parent_message_receipts = block.parent_state_root = state;
        put(ipld, nullptr, block);
        OUTCOME_EXCEPT(cr.expandTipset(block));
      }
      std::vector<MessageReceipt> receipts;
      auto tipset{cr.getTipset(true)};
      OUTCOME_EXCEPT(res, vmi.applyBlocks(nullptr, tipset, &receipts));
      state = res.state_root;
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
}

void testMessages(const MessageVector &mv, IpldPtr ipld) {
  for (const auto &precondition : mv.precondition_variants) {
    BlockHeader b;
    b.ticket.emplace();
    b.messages = b.parent_message_receipts = b.parent_state_root =
        mv.state_before;
    b.parent_base_fee = mv.parent_base_fee;
    OUTCOME_EXCEPT(ts, Tipset::create({b}));
    std::shared_ptr<Invoker> invoker = std::make_shared<InvokerImpl>();
    std::shared_ptr<RuntimeRandomness> randomness =
        std::make_shared<ReplayingRandomness>(mv.randomness);
    fc::vm::runtime::EnvironmentContext env_context{ipld, invoker, randomness};
    auto env{std::make_shared<fc::vm::runtime::Env>(env_context, nullptr, ts)};
    auto i{0};
    for (const auto &[epoch_offset, message] : mv.messages) {
      const auto &receipt{mv.receipts[i]};
      env->epoch = precondition.epoch + epoch_offset;
      auto size = message.from.isSecp256k1()
                      ? fc::vm::message::SignedMessage{message,
                                                       fc::crypto::signature::
                                                           Secp256k1Signature{}}
                            .chainSize()
                      : message.chainSize();
      OUTCOME_EXCEPT(apply, env->applyMessage(message, size));
      EXPECT_EQ(apply.receipt.exit_code, receipt.exit_code);
      EXPECT_EQ(apply.receipt.return_value, receipt.return_value);
      EXPECT_EQ(apply.receipt.gas_used, receipt.gas_used);
      ++i;
    }
    OUTCOME_EXCEPT(state, env->state_tree->flush());
    EXPECT_EQ(state, mv.state_after);
  }
}

TEST_P(TestVectors, Vector) {
  auto &mv{GetParam()};
  fc::vm::actor::cgo::configParams();

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
                        [](auto &&p) { return testName(p.param.path); });
