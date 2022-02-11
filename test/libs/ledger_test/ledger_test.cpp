/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/wallet/ledger.hpp"

#include <gtest/gtest.h>
#include <iostream>

#include "codec/cbor/cbor_codec.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/keystore/keystore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/message/impl/message_signer_impl.hpp"
#include "vm/message/message.hpp"

namespace fc::api {
  using primitives::address::encodeToString;
  using storage::InMemoryStorage;
  using vm::actor::MethodNumber;
  using vm::actor::MethodParams;
  using vm::message::MessageSignerImpl;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  struct LedgerTest : testing::Test {
    void SetUp() override {
      primitives::address::currentNetwork = primitives::address::MAINNET;
    }

    void PrintBytes(const Bytes &bytes) {
      for (const auto byte : bytes) {
        std::cout << std::setfill('0') << std::setw(2) << std::hex << int(byte)
                  << " ";
      }
      std::cout << std::endl;
    }

    std::shared_ptr<InMemoryStorage> store =
        std::make_shared<InMemoryStorage>();
    MessageSignerImpl signer{storage::keystore::kDefaultKeystore};
  };

  TEST_F(LedgerTest, CheckLedger) {
    const Ledger ledger(store);

    std::cout << "===============================================" << std::endl;
    std::cout << "Ledger physical device test" << std::endl;
    std::cout << " - Ledger device must be connected via USB and unlocked"
              << std::endl;
    std::cout << " - Filecoin application must be opened on Ledger device"
              << std::endl;
    std::cout << " - Be ready to approve some actions on Ledger device"
              << std::endl;
    std::cout << "===============================================" << std::endl;

    std::cout << ">>> Check New method" << std::endl;
    std::cout << "> Approve address on Ledger device" << std::endl;

    EXPECT_OUTCOME_TRUE(address, ledger.New());

    std::cout << "New address created and imported: " << encodeToString(address)
              << std::endl
              << std::endl;

    std::cout << ">>> Check Has method" << std::endl;

    EXPECT_OUTCOME_EQ(ledger.Has(address), true);

    std::cout << "Ledger exists the address: " << encodeToString(address)
              << std::endl
              << std::endl;

    const UnsignedMessage message(Address{1000},
                                  Address{1001},
                                  0,
                                  1,
                                  0,
                                  1,
                                  MethodNumber{0},
                                  MethodParams{""_unhex});

    EXPECT_OUTCOME_TRUE(data, codec::cbor::encode(message));

    std::cout << ">>> Check Sign method" << std::endl;
    std::cout << "Message: ";
    PrintBytes(data);
    std::cout << "> Approve signing on Ledger device" << std::endl;

    EXPECT_OUTCOME_TRUE(signature, ledger.Sign(address, data));

    std::cout << "Message is signed. Signature: " << std::endl;
    PrintBytes(signature.toBytes());

    const SignedMessage sm{message, signature};

    EXPECT_OUTCOME_TRUE(result, signer.verify(address, sm));
    EXPECT_EQ(result, message);
    if (result == message) {
      std::cout << "Signature is verified" << std::endl;
    }

    std::cout << "===============================================" << std::endl;
  }

}  // namespace fc::api
