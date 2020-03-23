/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <gtest/gtest.h>
#include "blockchain/block_validator/impl/block_validator_impl.hpp"
#include "clock/impl/chain_epoch_clock_impl.hpp"
#include "power/impl/power_table_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/blockchain/weight_calculator_mock.hpp"
#include "testutil/mocks/clock/utc_clock_mock.hpp"
#include "testutil/mocks/crypto/bls/bls_provider_mock.hpp"
#include "testutil/mocks/crypto/secp256k1/secp256k1_provider_mock.hpp"
#include "testutil/mocks/storage/ipfs/ipfs_datastore_mock.hpp"
#include "testutil/mocks/vm/indices/indices_mock.hpp"
#include "testutil/mocks/vm/interpreter/interpreter_mock.hpp"
#include "testutil/outcome.hpp"

namespace config {
  constexpr size_t kGenesisTime{7000};
  constexpr uint64_t kMinerId{1};
  constexpr uint64_t kMinerPower{888};
  const auto b96 =
      "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;
}  // namespace config

class BlockValidatorTest : public testing::Test {
 protected:
  using BlockValidator = fc::blockchain::block_validator::BlockValidatorImpl;
  using DataStore = fc::storage::ipfs::MockIpfsDatastore;
  using EpochClock = fc::clock::ChainEpochClockImpl;
  using WeightCalculator = fc::blockchain::weight::WeightCalculatorMock;
  using PowerTable = fc::power::PowerTableImpl;
  using BlsProvider = fc::crypto::bls::BlsProviderMock;
  using Secp256k1Provider = fc::crypto::secp256k1::Secp256k1ProviderMock;
  using Indices = fc::vm::indices::MockIndices;
  using Interpreter = fc::vm::interpreter::InterpreterMock;
  using BlockHeader = fc::primitives::block::BlockHeader;
  using Address = fc::primitives::address::Address;
  using Ticket = fc::primitives::ticket::Ticket;
  using Signature = fc::crypto::signature::Signature;
  using Secp256k1Signature = fc::crypto::signature::Secp256k1Signature;

  BlockValidatorTest() : validator_{createValidator()} {}

  std::shared_ptr<BlockValidator> validator_;

  std::shared_ptr<BlockValidator> createValidator() {
    auto datastore = std::make_shared<DataStore>();
    auto utc_clock = std::make_shared<UTCClockMock>();
    std::chrono::duration<uint64_t> genesis_time{config::kGenesisTime};
    auto epoch_clock = std::make_shared<EpochClock>(Time{genesis_time});
    auto weight_calculator = std::make_shared<WeightCalculator>();
    auto power_table = std::make_shared<PowerTable>();
    auto result = power_table->setMinerPower(
        Address::makeFromId(config::kMinerId), config::kMinerPower);
    BOOST_ASSERT(!result.has_error());
    auto bls_provider = std::make_shared<BlsProvider>();
    auto secp_provider = std::make_shared<Secp256k1Provider>();
    auto vm_interpreter = std::make_shared<Interpreter>();
    auto vm_indices = std::make_shared<Indices>();
    return std::make_shared<BlockValidator>(datastore,
                                            utc_clock,
                                            epoch_clock,
                                            weight_calculator,
                                            power_table,
                                            bls_provider,
                                            secp_provider,
                                            vm_interpreter,
                                            vm_indices);
  }

  BlockHeader getCorrectBlockHeader() const {
    return {Address::makeFromId(1),
            Ticket{config::b96},
            {fc::common::Buffer{"DEAD"_unhex}, config::b96, {}},
            {"010001020002"_cid},
            3,
            4,
            "010001020005"_cid,
            "010001020006"_cid,
            "010001020007"_cid,
            "DEAD"_unhex,
            8,
            Signature{Secp256k1Signature{"DEAD"_unhex}},
            9};
  }
};

/**
 * @given Correct block
 * @when Validating correct block
 * @then Valiodation must be successful
 */
TEST_F(BlockValidatorTest, ValidateCorrectBlock) {
  EXPECT_OUTCOME_TRUE_1(validator_->validateBlock(
      getCorrectBlockHeader(),
      {fc::blockchain::block_validator::scenarios::Stage::SYNTAX_BV0}));
}
