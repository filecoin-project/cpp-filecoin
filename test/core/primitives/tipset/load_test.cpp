/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/load.hpp"

#include "storage/ipfs/impl/in_memory_datastore.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

namespace fc::primitives::tipset {

  using storage::ipfs::InMemoryDatastore;

  struct CacheLoadTest : public ::testing::Test {
    void SetUp() override {
      ipld_ = std::make_shared<InMemoryDatastore>();

      std::vector<BlockHeader> headers = {{}, {}, {}, {}};
      for (size_t i = 0; i < headers.size(); ++i) {
        headers[i].miner = Address::makeFromId(i);
        headers[i].parent_state_root = "010001020005"_cid;
        headers[i].parent_message_receipts = "010001020005"_cid;
        headers[i].messages = "010001020005"_cid;

        const auto cid{*asBlake(put(ipld_, nullptr, headers[i]))};

        keys_.push_back(TipsetKey({cid}));
      }

      ipld_load_ = std::make_shared<TsLoadIpld>(
          ipld_);  // TODO(ortyomka): should be mock
      size_ = 3;
      cache_load_ = std::make_shared<TsLoadCache>(ipld_load_, size_);
    }

   protected:
    size_t size_;
    std::vector<TipsetKey> keys_;
    std::shared_ptr<InMemoryDatastore> ipld_;
    std::shared_ptr<TsLoadIpld> ipld_load_;
    std::shared_ptr<TsLoadCache> cache_load_;
  };

  /**
   * @given cache load, 4 tipset
   * @when load tipsets and check indexes
   * @then last was removed
   */
  TEST_F(CacheLoadTest, Load) {
    for (size_t i = 0; i < keys_.size() * 2; ++i) {
      EXPECT_OUTCOME_TRUE(
          res, cache_load_->loadWithCacheInfo(keys_[i % keys_.size()]));
      ASSERT_EQ(res.index, i % 3);
    }
  }

}  // namespace fc::primitives::tipset
