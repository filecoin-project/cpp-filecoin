/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "vm/interpreter/interpreter.hpp"

#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/cbor_blake/cbor_blake_ipld_mock.hpp"
#include "testutil/outcome.hpp"

namespace fc::vm::interpreter {
  using storage::InMemoryStorage;
  using ::testing::Eq;
  using ::testing::Return;

  TEST(InterpreterCacheTest, CachedStateRootAbsent) {
    auto ipld = std::make_shared<CborBlakeIpldMock>();
    auto interpreter_cache{std::make_shared<InterpreterCache>(
        std::make_shared<InMemoryStorage>(), ipld)};

    const CID state_root{CbCid{}};
    const Result result{.state_root = state_root,
                        .message_receipts = "010001020003"_cid,
                        .weight = 1};
    const TipsetKey tipset_key;
    interpreter_cache->set(InterpreterCache::Key{tipset_key}, result);

    EXPECT_CALL(*ipld, get(Eq(asBlake(state_root)), Eq(nullptr)))
        .WillOnce(Return(false));

    const auto res =
        interpreter_cache->tryGet(InterpreterCache::Key{tipset_key});
    EXPECT_FALSE(res.has_value());
  }
}  // namespace fc::vm::interpreter
