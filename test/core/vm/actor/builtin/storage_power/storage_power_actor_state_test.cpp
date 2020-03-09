/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/vm/actor/builtin/storage_power/storage_power_actor_state.hpp"

#include "filecoin/storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/crypto/randomness/randomness_provider_mock.hpp"
#include "testutil/mocks/vm/indices/indices_mock.hpp"
#include "testutil/outcome.hpp"
#include "filecoin/vm/exit_code/exit_code.hpp"

using fc::CID;
using fc::adt::BalanceTableHamt;
using fc::crypto::randomness::MockRandomnessProvider;
using fc::crypto::randomness::Randomness;
using fc::crypto::randomness::RandomnessProvider;
using fc::power::Power;
using fc::primitives::ChainEpoch;
using fc::primitives::TokenAmount;
using fc::primitives::address::Address;
using fc::storage::hamt::Hamt;
using fc::storage::hamt::HamtError;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipfs::IpfsDatastore;
using fc::vm::VMExitCode;
using fc::vm::actor::builtin::storage_power::Claim;
using fc::vm::actor::builtin::storage_power::CronEvent;
using fc::vm::actor::builtin::storage_power::kConsensusMinerMinPower;
using fc::vm::actor::builtin::storage_power::StoragePowerActor;
using fc::vm::actor::builtin::storage_power::StoragePowerActorState;
using fc::vm::indices::Indices;
using fc::vm::indices::MockIndices;
using testing::_;
using namespace std::string_literals;

class StoragePowerActorStateTest : public ::testing::Test {
 public:
  std::shared_ptr<IpfsDatastore> datastore =
      std::make_shared<InMemoryDatastore>();
  std::shared_ptr<StoragePowerActor> actor;

  Address addr{Address::makeFromId(3232104785)};

  Claim default_claim{.power = 1, .pledge = 0};

  void SetUp() override {
    EXPECT_OUTCOME_TRUE(state, StoragePowerActor::createEmptyState(datastore));
    actor = std::make_shared<StoragePowerActor>(datastore, state);
  }
};

/**
 * @given Storage Power Actor and 1 miner
 * @when try to add same miner again
 * @then error ALREADY_EXIST
 */
TEST_F(StoragePowerActorStateTest, AddMiner_Twice) {
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT,
                       actor->getClaim(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ACTOR_ALREADY_EXISTS,
                       actor->addMiner(addr));
}

/**
 * @given Storage Power Actor and 1 miner
 * @when try to delete the miner
 * @then miner successfully deleted
 */
TEST_F(StoragePowerActorStateTest, deleteMiner_Success) {
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT,
                       actor->getClaim(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE(claim, actor->getClaim(addr));
  EXPECT_EQ(claim.power, 0);
  EXPECT_EQ(claim.pledge, 0);
  EXPECT_OUTCOME_TRUE_1(actor->deleteMiner(addr));
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT,
                       actor->getClaim(addr));
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT,
                       actor->getMinerBalance(addr));
}

/**
 * @given Storage Power Actor
 * @when try to delete no existen miner
 * @then error NO_SUCH_MINER
 */
TEST_F(StoragePowerActorStateTest, DeleteMiner_NoMiner) {
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT,
                       actor->deleteMiner(addr));
}

/**
 * @given Storage Power Actor
 * @when try to add power to miner
 * @then power successfully added
 */
TEST_F(StoragePowerActorStateTest, addClaimedPower_Success) {
  Claim empty_claim{.power = 0, .pledge = 0};

  fc::power::Power min_candidate_storage_value = kConsensusMinerMinPower;

  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->setClaim(addr, empty_claim));
  EXPECT_OUTCOME_TRUE_1(
      actor->addToClaim(addr, min_candidate_storage_value, 0));
  EXPECT_OUTCOME_TRUE(claim, actor->getClaim(addr));
  EXPECT_EQ(claim.power, min_candidate_storage_value);
  EXPECT_OUTCOME_EQ(actor->computeNominalPower(addr),
                    min_candidate_storage_value);
}

/**
 * @given Storage Power Actor
 * @when try to add sector power to miner
 * @then power successfully added
 */
TEST_F(StoragePowerActorStateTest,
       addClaimedPowerForSectorSuccess_ButLessThatMinCandidateStorage) {
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->setClaim(addr, default_claim));
  EXPECT_OUTCOME_TRUE_1(actor->addToClaim(addr, 10, 0));

  EXPECT_OUTCOME_EQ(actor->computeNominalPower(addr), 11);
  EXPECT_OUTCOME_TRUE(claim, actor->getClaim(addr));
  EXPECT_EQ(claim.power, Power{11});
}

/**
 * @given Storage Power Actor and sector
 * @when try to add sector power to miner, but miner fail proof of space time
 * @then power successfully added, but nominal is 0
 */
TEST_F(StoragePowerActorStateTest, addClaimedPowerForSector_FailPoSt) {
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->setClaim(addr, default_claim));
  EXPECT_OUTCOME_TRUE_1(actor->addFaultMiner(addr));
  EXPECT_OUTCOME_EQ(actor->computeNominalPower(addr), Power{0});
}

/**
 * @given Storage Power Actor
 * @when try to deduct balance from miner
 * @then power successfully deducted
 */
TEST_F(StoragePowerActorStateTest, deductClaimedPowerForSectorAssert_Success) {
  TokenAmount amount_to_add{222};
  TokenAmount floor{0};

  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addMinerBalance(addr, amount_to_add));
  EXPECT_OUTCOME_EQ(actor->subtractMinerBalance(addr, amount_to_add, floor),
                    amount_to_add);
}

/**
 * @given populated state
 * @when try to serialize and then deserialize
 * @then state is preserved
 */
TEST_F(StoragePowerActorStateTest, CBOR) {
  ChainEpoch epoch{12345};
  CronEvent event{};
  EXPECT_OUTCOME_TRUE_1(actor->appendCronEvent(epoch, event));

  Address address_1{Address::makeFromId(1)};
  TokenAmount balance_1 = 111;
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(address_1));
  EXPECT_OUTCOME_TRUE_1(actor->setMinerBalance(address_1, balance_1));

  Address address_2{Address::makeFromId(2)};
  TokenAmount balance_2 = 22;
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(address_2));
  EXPECT_OUTCOME_TRUE_1(actor->setMinerBalance(address_2, balance_2));
  EXPECT_OUTCOME_TRUE_1(actor->setClaim(address_2, default_claim));

  Address address_3{Address::makeFromId(3)};
  TokenAmount balance_3 = 333;
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(address_3));
  EXPECT_OUTCOME_TRUE_1(actor->setMinerBalance(address_3, balance_3));
  EXPECT_OUTCOME_TRUE_1(actor->addFaultMiner(address_3));

  fc::codec::cbor::CborEncodeStream encoder;
  EXPECT_OUTCOME_TRUE(state, actor->flushState())
  encoder << state;
  fc::codec::cbor::CborDecodeStream decoder(encoder.data());
  StoragePowerActorState new_state;
  decoder >> new_state;
  StoragePowerActor new_actor(datastore, new_state);

  EXPECT_OUTCOME_EQ(new_actor.getMinerBalance(address_1), balance_1);
  EXPECT_OUTCOME_EQ(new_actor.getMinerBalance(address_2), balance_2);
  EXPECT_OUTCOME_EQ(new_actor.getClaim(address_2), default_claim);
  EXPECT_OUTCOME_EQ(new_actor.getMinerBalance(address_3), balance_3);
  EXPECT_OUTCOME_EQ(new_actor.hasFaultMiner(address_3), true);
}
