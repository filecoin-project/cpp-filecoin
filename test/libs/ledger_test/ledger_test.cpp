/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/wallet/ledger.hpp"

#include <gtest/gtest.h>

#include "codec/cbor/cbor_codec.hpp"
#include "primitives/address/address.hpp"
#include "storage/keystore/keystore.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/map_prefix/prefix.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/message/message.hpp"

#include "primitives/address/address_codec.hpp"

namespace fc::api {
  using primitives::address::decodeFromString;
  using vm::actor::MethodNumber;
  using vm::actor::MethodParams;
  using vm::message::UnsignedMessage;

  struct LedgerTest : testing::Test {
    void SetUp() override {
      EXPECT_OUTCOME_TRUE(leveldb_res, storage::LevelDB::create("./leveldb"));

      store = std::move(leveldb_res);
    }

    MapPtr store;
  };

  TEST_F(LedgerTest, CheckLedger) {
    const Ledger ledger(store);

    EXPECT_OUTCOME_TRUE(address, ledger.New());

    EXPECT_OUTCOME_EQ(ledger.Has(address), true);

    const UnsignedMessage message(Address{1000},
                                  Address{1001},
                                  0,
                                  1,
                                  0,
                                  1,
                                  MethodNumber{0},
                                  MethodParams{""_unhex});

    EXPECT_OUTCOME_TRUE(data, message.getCid().toBytes());

    EXPECT_OUTCOME_TRUE(signature, ledger.Sign(address, data));

    EXPECT_OUTCOME_EQ(
        storage::keystore::kDefaultKeystore->verify(address, data, signature),
        true);
  }

}  // namespace fc::api
