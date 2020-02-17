/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/crypto/randomness/randomness_provider_mock.hpp"
#include "testutil/mocks/vm/indices/indices_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/exit_code/exit_code.hpp"

using fc::CID;
using fc::adt::BalanceTableHamt;
using fc::crypto::randomness::MockRandomnessProvider;
using fc::crypto::randomness::Randomness;
using fc::crypto::randomness::RandomnessProvider;
using fc::power::Power;
using fc::primitives::ChainEpoch;
using fc::primitives::address::Address;
using fc::storage::hamt::Hamt;
using fc::storage::hamt::HamtError;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipfs::IpfsDatastore;
using fc::vm::VMExitCode;
using fc::vm::actor::builtin::storage_power::Claim;
using fc::vm::actor::builtin::storage_power::CronEvent;
using fc::vm::actor::builtin::storage_power::kConsensusMinerMinPower;
using fc::vm::actor::builtin::storage_power::StoragePowerActorState;
using fc::vm::actor::builtin::storage_power::TokenAmount;
using fc::vm::indices::Indices;
using fc::vm::indices::MockIndices;
using testing::_;
using namespace std::string_literals;

class StoragePowerActorTest : public ::testing::Test {
 public:
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();
  std::shared_ptr<IpfsDatastore> datastore =
      std::make_shared<InMemoryDatastore>();
  std::shared_ptr<StoragePowerActorState> actor_state;

  Address addr{Address::makeFromId(3232104785)};

  Claim default_claim{.power = 1, .pledge = 0};

  void SetUp() override {
    Hamt empty_hamt(datastore);
    CID cidEmpty = empty_hamt.flush().value();
    actor_state = std::make_shared<StoragePowerActorState>(indices,
                                                           randomness_provider,
                                                           datastore,
                                                           cidEmpty,
                                                           cidEmpty,
                                                           cidEmpty,
                                                           cidEmpty);
  }
};

/**
 * @given Storage Power Actor and 1 miner
 * @when try to add same miner again
 * @then error ALREADY_EXIST
 */
TEST_F(StoragePowerActorTest, AddMiner_Twice) {
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND,
                       actor_state->getClaim(addr));
  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr));
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ACTOR_ALREADY_EXISTS,
                       actor_state->addMiner(addr));
}

/**
 * @given Storage Power Actor and 1 miner
 * @when try to delete the miner
 * @then miner successfully deleted
 */
TEST_F(StoragePowerActorTest, deleteMiner_Success) {
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND,
                       actor_state->getClaim(addr));
  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr));
  EXPECT_OUTCOME_TRUE(claim, actor_state->getClaim(addr));
  EXPECT_EQ(claim.power, 0);
  EXPECT_EQ(claim.pledge, 0);
  EXPECT_OUTCOME_TRUE_1(actor_state->deleteMiner(addr));
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND,
                       actor_state->getClaim(addr));
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND,
                       actor_state->getMinerBalance(addr));
}

/**
 * @given Storage Power Actor
 * @when try to delete no existen miner
 * @then error NO_SUCH_MINER
 */
TEST_F(StoragePowerActorTest, DeleteMiner_NoMiner) {
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ACTOR_NOT_FOUND,
                       actor_state->deleteMiner(addr));
}

/**
 * @given Storage Power Actor
 * @when try to add power to miner
 * @then power successfully added
 */
TEST_F(StoragePowerActorTest, addClaimedPower_Success) {
  Claim empty_claim{.power = 0, .pledge = 0};

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  fc::power::Power min_candidate_storage_value = kConsensusMinerMinPower;

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(min_candidate_storage_value));

  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr, empty_claim));
  EXPECT_OUTCOME_TRUE_1(
      actor_state->addToClaim(addr, min_candidate_storage_value, 0));
  EXPECT_OUTCOME_TRUE(claim, actor_state->getClaim(addr));
  EXPECT_EQ(claim.power, min_candidate_storage_value);
  EXPECT_OUTCOME_EQ(actor_state->computeNominalPower(addr),
                    min_candidate_storage_value);
}

/**
 * @given Storage Power Actor
 * @when try to add sector power to miner
 * @then power successfully added
 */
TEST_F(StoragePowerActorTest,
       addClaimedPowerForSectorSuccess_ButLessThatMinCandidateStorage) {
  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(100));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr, default_claim));
  EXPECT_OUTCOME_TRUE_1(actor_state->addToClaim(addr, 10, 0));

  EXPECT_OUTCOME_EQ(actor_state->computeNominalPower(addr), 11);
  EXPECT_OUTCOME_TRUE(claim, actor_state->getClaim(addr));
  EXPECT_EQ(claim.power, Power{11});
}

/**
 * @given Storage Power Actor and sector
 * @when try to add sector power to miner, but miner fail proof of space time
 * @then power successfully added, but nominal is 0
 */
TEST_F(StoragePowerActorTest, addClaimedPowerForSector_FailPoSt) {
  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr, default_claim));
  EXPECT_OUTCOME_TRUE_1(actor_state->addFaultMiner(addr));
  EXPECT_OUTCOME_EQ(actor_state->computeNominalPower(addr), Power{0});
}

/**
 * @given Storage Power Actor
 * @when try to deduct balance from miner
 * @then power successfully deducted
 */
TEST_F(StoragePowerActorTest, deductClaimedPowerForSectorAssert_Success) {
  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  TokenAmount amount_to_add{222};
  TokenAmount floor{0};

  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor_state->addMinerBalance(addr, amount_to_add));
  EXPECT_OUTCOME_EQ(
      actor_state->subtractMinerBalance(addr, amount_to_add, floor),
      amount_to_add);
}

/**
 * @given populated state
 * @when try to serialize and then deserialize
 * @then state is preserved
 */
TEST_F(StoragePowerActorTest, CBOR) {
  ChainEpoch epoch{12345};
  CronEvent event{};
  EXPECT_OUTCOME_TRUE_1(actor_state->appendCronEvent(epoch, event));

  Address address_1{Address::makeFromId(1)};
  TokenAmount balance_1 = 111;
  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(address_1));
  EXPECT_OUTCOME_TRUE_1(actor_state->setMinerBalance(address_1, balance_1));

  Address address_2{Address::makeFromId(2)};
  TokenAmount balance_2 = 22;
  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(address_2));
  EXPECT_OUTCOME_TRUE_1(actor_state->setMinerBalance(address_2, balance_2));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(address_2, default_claim));

  Address address_3{Address::makeFromId(3)};
  TokenAmount balance_3 = 333;
  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(address_3));
  EXPECT_OUTCOME_TRUE_1(actor_state->setMinerBalance(address_3, balance_3));
  EXPECT_OUTCOME_TRUE_1(actor_state->addFaultMiner(address_3));

  fc::codec::cbor::CborEncodeStream encoder;
  encoder << *actor_state;
  fc::codec::cbor::CborDecodeStream decoder(encoder.data());
  decoder >> *actor_state;

  EXPECT_OUTCOME_EQ(actor_state->getMinerBalance(address_1), balance_1);
  EXPECT_OUTCOME_EQ(actor_state->getMinerBalance(address_2), balance_2);
  EXPECT_OUTCOME_EQ(actor_state->getClaim(address_2), default_claim);
  EXPECT_OUTCOME_EQ(actor_state->getMinerBalance(address_3), balance_3);
  EXPECT_OUTCOME_EQ(actor_state->hasFaultMiner(address_3), true);
}
