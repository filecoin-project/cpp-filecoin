/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/file.hpp"

#include <gtest/gtest.h>

#include "cbor_blake/ipld_any.hpp"
#include "cbor_blake/memory.hpp"
#include "codec/cbor/light_reader/block.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::primitives::tipset::chain::file {
  struct FileTest : ::test::BaseFS_Test {
    CbIpldPtr ipld{std::make_shared<MemoryCbIpld>()};
    std::vector<Bytes> tickets{{"02"_unhex}, {"03"_unhex}, {"01"_unhex}};
    CbCid genesis{makeBlock(0, 0, {})};
    BlockParentCbCids head0, head00, head01, head010, head011, head0110;
    std::string path, path_hash, path_count;
    TsBranchPtr branch;
    bool updated{};

    CbCid makeBlock(ChainEpoch height,
                    uint64_t miner,
                    const BlockParentCbCids &parents) {
      BlockHeader block;
      assert(miner < tickets.size());
      block.miner = Address::makeFromId(miner);
      block.ticket.emplace(block::Ticket{tickets[miner]});
      block.parents = parents;
      block.height = height;
      block.parent_state_root = block.parent_message_receipts = block.messages =
          "010001020005"_cid;
      return ipld->put(codec::cbor::encode(block).value());
    }

    BlockParentCbCids makeTs(ChainEpoch height,
                             const std::set<uint64_t> &miners,
                             const BlockParentCbCids &parents) {
      BlockParentCbCids cids;
      for (auto &miner : miners) {
        cids.push_back(makeBlock(height, miner, parents));
      }
      return cids;
    }

    FileTest()
        : BaseFS_Test{"file_test"},
          path{(getPathString() / "chain").string()},
          path_hash{path + ".hash"},
          path_count{path + ".count"} {
      for (size_t i{1}; i < tickets.size(); ++i) {
        EXPECT_LT(crypto::blake2b::blake2b_256(tickets[i - 1]),
                  crypto::blake2b::blake2b_256(tickets[i]));
      }
      head0 = makeTs(2, {0}, {genesis});
      head00 = makeTs(4, {0}, head0);
      head01 = makeTs(4, {0, 1}, head0);
      head010 = makeTs(6, {0}, head01);
      head011 = makeTs(6, {0, 1}, head01);
      head0110 = makeTs(7, {0}, head011);
    }

    void load(const std::vector<CbCid> &head, size_t lazy_limit = 0) {
      branch = loadOrCreate(
          &updated, path, ipld, head, head.empty() ? 0 : 1, lazy_limit);
      EXPECT_TRUE(branch);
    }

    void checkChain(const std::vector<CbCid> &head) {
      EXPECT_FALSE(branch->chain.empty());
      EXPECT_EQ(branch->chain.rbegin()->second.key.cids(), head);
      const std::vector<CbCid> *expected_parents{};
      BlockParentCbCids actual_parents;
      Bytes _block;
      for (auto it{branch->chain.begin()}; it != branch->chain.end(); ++it) {
        for (auto &cid : it->second.key.cids()) {
          EXPECT_TRUE(ipld->get(cid, _block));
          BytesIn block{_block};
          BytesIn ticket;
          ChainEpoch height{};
          EXPECT_TRUE(codec::cbor::light_reader::readBlock(
              ticket, actual_parents, height, block));
          EXPECT_EQ(height, it->first);
          if (expected_parents) {
            EXPECT_EQ(actual_parents, *expected_parents);
          } else {
            EXPECT_EQ(height, 0);
          }
        }
        expected_parents = &it->second.key.cids();
      }
    }

    void update(const std::vector<CbCid> &head) {
      EXPECT_OUTCOME_TRUE(
          branch2,
          TsBranch::make(
              std::make_shared<TsLoadIpld>(std::make_shared<CbAsAnyIpld>(ipld)),
              head,
              branch));
      EXPECT_OUTCOME_TRUE_1(
          chain::update(branch, {branch2, std::prev(branch2->chain.end())}));
      EXPECT_TRUE(branch->updater->flush());
    }

    void cut(const std::string &path, size_t n) {
      fs::resize_file(path, fs::file_size(path) - n);
    }
  };

  TEST_F(FileTest, Flow) {
    // create
    load(head00);
    EXPECT_TRUE(updated);
    checkChain(head00);

    // update
    update(head010);
    load(head00);
    EXPECT_FALSE(updated);
    checkChain(head010);

    // load, update
    load(head0110);
    EXPECT_TRUE(updated);
    checkChain(head0110);
    load({});

    // truncate hash
    fs::resize_file(path_hash, fs::file_size(path_hash) - 1);
    load({});
    EXPECT_FALSE(updated);
    checkChain(head011);

    // truncate count
    fs::resize_file(path_count, fs::file_size(path_count) - 1);
    load({});
    EXPECT_FALSE(updated);
    checkChain(head01);

    // missing count
    fs::remove(path_count);
    load(head00);
    EXPECT_TRUE(updated);
    checkChain(head00);
  }

  TEST_F(FileTest, Lazy) {
    auto make{[&](uint64_t miner, std::set<ChainEpoch> heights) {
      BlockParentCbCids head{{head00}};
      for (auto &height : heights) {
        head = makeTs(height, {miner}, head);
      }
      return head;
    }};
    auto head2{make(0, {6, 7, 9})};
    load(head2);
    auto head3{make(1, {6, 7, 9, 10})};
    load(head3, 1);
    EXPECT_TRUE(updated);
    checkChain(head3);

    load({}, 1);
    EXPECT_EQ(branch->chain.size(), 1);
    branch->lazy->min_load = 1;
    update(head2);
    EXPECT_EQ(branch->chain.size(), 4);
    branch->lazyLoad(0);
    checkChain(head2);
  }
}  // namespace fc::primitives::tipset::chain::file
