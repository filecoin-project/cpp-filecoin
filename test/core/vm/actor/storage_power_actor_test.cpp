/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/storage_power_actor.hpp"
#include "power/power_table_error.hpp"
#include "testutil/mocks/crypto/randomness/randomness_provider_mock.hpp"
#include "testutil/mocks/vm/indices/indices_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/exit_code/exit_code.hpp"

using fc::crypto::randomness::MockRandomnessProvider;
using fc::crypto::randomness::Randomness;
using fc::crypto::randomness::RandomnessProvider;
using fc::power::PowerTableError;
using fc::primitives::address::Address;
using fc::primitives::address::Network;
using fc::vm::VMExitCode;
using fc::vm::actor::StoragePowerActor;
using fc::vm::indices::Indices;
using fc::vm::indices::MockIndices;
using testing::_;

class StoragePowerActorTest : public ::testing::Test {
 public:
  void SetUp() override {
    indices = std::make_shared<MockIndices>();
    randomness_provider = std::make_shared<MockRandomnessProvider>();
    actor = std::make_shared<StoragePowerActor>(indices, randomness_provider);
  }

  std::shared_ptr<StoragePowerActor> actor;
  std::shared_ptr<MockIndices> indices;
  std::shared_ptr<MockRandomnessProvider> randomness_provider;

  fc::vm::actor::SectorStorageWeightDesc swd;

  Address addr{Address::makeFromId(3232104785)};
  Address addr_1{Address::makeFromId(323210478)};
  Address addr_2{Address::makeFromId(32321047)};
};

/**
 * @given Storage Power Actor and 1 miner
 * @when try to add same miner again
 * @then error ALREADY_EXIST
 */
TEST_F(StoragePowerActorTest, AddMiner_Twice) {
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       actor->getPowerTotalForMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ACTOR_ALREADY_EXISTS,
                       actor->addMiner(addr));
}

/**
 * @given Storage Power Actor and 1 miner
 * @when try to remove the miner
 * @then miner successfully removed
 */
TEST_F(StoragePowerActorTest, RemoveMiner_Success) {
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       actor->getPowerTotalForMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_EQ(actor->getPowerTotalForMiner(addr), 0);
  EXPECT_OUTCOME_TRUE_1(actor->removeMiner(addr));
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       actor->getPowerTotalForMiner(addr));
}

/**
 * @given Storage Power Actor
 * @when try to remove no existen miner
 * @then error NO_SUCH_MINER
 */
TEST_F(StoragePowerActorTest, RemoveMiner_NoMiner) {
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       actor->removeMiner(addr));
}

/**
 * @given Storage Power Actor and sector
 * @when try to add sector power to miner
 * @then power successfully added
 */
TEST_F(StoragePowerActorTest, addClaimedPowerForSector_Success) {
  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  fc::primitives::BigInt min_candidate_storage_value =
      StoragePowerActor::kMinMinerSizeStor;

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(min_candidate_storage_value));

  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addClaimedPowerForSector(addr, swd));
  EXPECT_OUTCOME_EQ(actor->getClaimedPowerForMiner(addr),
                    min_candidate_storage_value);
  EXPECT_OUTCOME_EQ(actor->getNominalPowerForMiner(addr),
                    min_candidate_storage_value);
  EXPECT_OUTCOME_EQ(actor->getPowerTotalForMiner(addr),
                    min_candidate_storage_value);
}

/**
 * @given Storage Power Actor and sector
 * @when try to add sector power to miner, but less that need for consensus
 * @then power successfully added, but total power is 0
 */
TEST_F(StoragePowerActorTest,
       addClaimedPowerForSectorSuccess_ButLessThatMinCandidateStorage) {
  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addClaimedPowerForSector(addr, swd));
  EXPECT_OUTCOME_EQ(actor->getClaimedPowerForMiner(addr), 1);
  EXPECT_OUTCOME_EQ(actor->getNominalPowerForMiner(addr), 1);
  EXPECT_OUTCOME_EQ(actor->getPowerTotalForMiner(addr), 0);
}

/**
 * @given Storage Power Actor and sector
 * @when try to add sector power to miner, but miner fail proof of space time
 * @then power successfully added, but nominal and total power is 0
 */
TEST_F(StoragePowerActorTest, addClaimedPowerForSector_FailPoSt) {
  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addFaultMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addClaimedPowerForSector(addr, swd));
  EXPECT_OUTCOME_EQ(actor->getClaimedPowerForMiner(addr), 1);
  EXPECT_OUTCOME_EQ(actor->getNominalPowerForMiner(addr), 0);
  EXPECT_OUTCOME_EQ(actor->getPowerTotalForMiner(addr), 0);
}

/**
 * @given Storage Power Actor and sector
 * @when try to deduct sector power from miner
 * @then power successfully deducted
 */
TEST_F(StoragePowerActorTest, deductClaimedPowerForSectorAssert_Success) {
  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addClaimedPowerForSector(addr, swd));
  EXPECT_OUTCOME_TRUE_1(actor->deductClaimedPowerForSectorAssert(addr, swd));
  EXPECT_OUTCOME_EQ(actor->getClaimedPowerForMiner(addr), 0);
  EXPECT_OUTCOME_EQ(actor->getNominalPowerForMiner(addr), 0);
  EXPECT_OUTCOME_EQ(actor->getPowerTotalForMiner(addr), 0);
}

/**
 * @given Storage Power Actor and 3 miner and randomness
 * @when try to choose 2 miners
 * @then 2 miners successfully chosen
 */
TEST_F(StoragePowerActorTest, selectMinersToSurprise_Success) {
  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  Randomness randomness;
  EXPECT_CALL(*randomness_provider, randomInt(randomness, _, _))
      .WillOnce(testing::Return(0))
      .WillOnce(testing::Return(0))
      .WillOnce(testing::Return(2))
      .WillRepeatedly(testing::Return(1));

  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr_1));
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr_2));

  EXPECT_OUTCOME_TRUE(miners, actor->getMiners());

  EXPECT_OUTCOME_TRUE(sup_miners, actor->selectMinersToSurprise(2, randomness));

  ASSERT_THAT(sup_miners, testing::ElementsAre(miners[0], miners[2]));
}

/**
 * @given Storage Power Actor and 3 miner and randomness
 * @when try to choose 3 miners
 * @then all miners return
 */
TEST_F(StoragePowerActorTest, selectMinersToSurprise_All) {
  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  Randomness randomness;
  EXPECT_CALL(*randomness_provider, randomInt(randomness, _, _))
      .WillRepeatedly(testing::Return(0));

  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr_1));
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr_2));

  EXPECT_OUTCOME_TRUE(miners, actor->getMiners());

  EXPECT_OUTCOME_TRUE(sup_miners,
                      actor->selectMinersToSurprise(miners.size(), randomness));

  ASSERT_THAT(sup_miners, miners);
}

/**
 * @given Storage Power Actor and 3 miner and randomness
 * @when try to choose more that 3 miners
 * @then OUT_OF_BOUND error
 */
TEST_F(StoragePowerActorTest, selectMinersToSurprise_MoreThatHave) {
  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  Randomness randomness;
  EXPECT_CALL(*randomness_provider, randomInt(randomness, _, _))
      .WillRepeatedly(testing::Return(0));

  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr_1));
  EXPECT_OUTCOME_TRUE_1(actor->addMiner(addr_2));

  EXPECT_OUTCOME_TRUE(miners, actor->getMiners());

  EXPECT_OUTCOME_ERROR(
      VMExitCode::STORAGE_POWER_ACTOR_OUT_OF_BOUND,
      actor->selectMinersToSurprise(miners.size() + 1, randomness));
}
