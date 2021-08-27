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
#include "testutil/mocks/vm/interpreter/interpreter_mock.hpp"
#include "testutil/outcome.hpp"

namespace config {
  constexpr size_t kGenesisTime{7000};
  constexpr uint64_t kMinerId{1};
  constexpr uint64_t kMinerPower{888};
  const auto b96 =
      "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;
}  // namespace config

namespace fc::blockchain::block_validator {
  using DataStore = storage::ipfs::MockIpfsDatastore;
  using EpochClock = clock::ChainEpochClockImpl;
  using WeightCalculator = weight::WeightCalculatorMock;
  using PowerTable = power::PowerTableImpl;
  using BlsProvider = crypto::bls::BlsProviderMock;
  using Secp256k1Provider = crypto::secp256k1::Secp256k1ProviderMock;
  using Interpreter = vm::interpreter::InterpreterMock;
  using crypto::signature::Secp256k1Signature;
  using crypto::signature::Signature;
  using primitives::address::Address;
  using primitives::block::BlockHeader;
  using primitives::block::Ticket;

  class BlockValidatorTest : public testing::Test {
   protected:
    BlockValidatorTest() : validator_{createValidator()} {}

    std::shared_ptr<BlockValidator> validator_;

    std::shared_ptr<BlockValidator> createValidator() {
      auto datastore = std::make_shared<DataStore>();
      auto utc_clock = std::make_shared<fc::clock::UTCClockMock>();
      auto epoch_clock =
          std::make_shared<EpochClock>(fc::clock::Time{config::kGenesisTime});
      auto weight_calculator = std::make_shared<WeightCalculator>();
      auto power_table = std::make_shared<PowerTable>();
      auto result = power_table->setMinerPower(
          Address::makeFromId(config::kMinerId), config::kMinerPower);
      BOOST_ASSERT(!result.has_error());
      auto bls_provider = std::make_shared<BlsProvider>();
      auto secp_provider = std::make_shared<Secp256k1Provider>();
      return std::make_shared<BlockValidatorImpl>(datastore,
                                                  utc_clock,
                                                  epoch_clock,
                                                  weight_calculator,
                                                  power_table,
                                                  bls_provider,
                                                  secp_provider,
                                                  nullptr);
    }

    BlockHeader getCorrectBlockHeader() const {
      return {Address::makeFromId(1),
              Ticket{fc::Buffer{config::b96}},
              {},
              {fc::primitives::block::BeaconEntry{
                  4,
                  Buffer{"F00D"_unhex},
              }},
              {fc::primitives::sector::PoStProof{
                  fc::primitives::sector::RegisteredPoStProof::
                      kStackedDRG2KiBWinningPoSt,
                  "F00D"_unhex,
              }},
              {fc::CbCid::hash("01"_unhex)},
              3,
              4,
              "010001020005"_cid,
              "010001020006"_cid,
              "010001020007"_cid,
              Signature{Secp256k1Signature{}},
              8,
              Signature{Secp256k1Signature{}},
              9,
              {}};
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

}  // namespace fc::blockchain::block_validator
