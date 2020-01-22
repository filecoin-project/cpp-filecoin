/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power/power_table_error.hpp"
#include "testutil/mocks/crypto/randomness/randomness_provider_mock.hpp"
#include "testutil/mocks/vm/indices/indices_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/storage_power_actor.hpp"

using fc::crypto::randomness::MockRandomnessProvider;
using fc::crypto::randomness::Randomness;
using fc::crypto::randomness::RandomnessProvider;
using fc::power::PowerTableError;
using fc::primitives::address::Address;
using fc::primitives::address::Network;
using fc::vm::Indices;
using fc::vm::MockIndices;
using fc::vm::actor::StoragePowerActor;
using testing::_;

/**
 * @given Storage Power Actor
 * @when try to add unique miner
 * @then miner successfully added
 */
TEST(StoragePowerActor, AddMiner_Success) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  StoragePowerActor actor(indices, randomness_provider);

  Address addr{Network::MAINNET, 3232104785};
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       actor.getPowerTotalForMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr));
  EXPECT_OUTCOME_EQ(actor.getPowerTotalForMiner(addr), 0);
}

/**
 * @given Storage Power Actor and 1 miner
 * @when try to add same miner again
 * @then error ALREADY_EXIST
 */
TEST(StoragePowerActor, AddMiner_Twice) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  StoragePowerActor actor(indices, randomness_provider);

  Address addr{Network::MAINNET, 3232104785};
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       actor.getPowerTotalForMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr));
  EXPECT_OUTCOME_ERROR(StoragePowerActor::ALREADY_EXIST, actor.addMiner(addr));
}

/**
 * @given Storage Power Actor and 1 miner
 * @when try to remove the miner
 * @then miner successfully removed
 */
TEST(StoragePowerActor, RemoveMiner_Success) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  StoragePowerActor actor(indices, randomness_provider);

  Address addr{Network::MAINNET, 3232104785};
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       actor.getPowerTotalForMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr));
  EXPECT_OUTCOME_EQ(actor.getPowerTotalForMiner(addr), 0);
  EXPECT_OUTCOME_TRUE_1(actor.removeMiner(addr));
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER,
                       actor.getPowerTotalForMiner(addr));
}

/**
 * @given Storage Power Actor
 * @when try to remove no existen miner
 * @then error NO_SUCH_MINER
 */
TEST(StoragePowerActor, RemoveMiner_NoMiner) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  StoragePowerActor actor(indices, randomness_provider);

  Address addr{Network::MAINNET, 3232104785};
  EXPECT_OUTCOME_ERROR(PowerTableError::NO_SUCH_MINER, actor.removeMiner(addr));
}

/**
 * @given Storage Power Actor and sector
 * @when try to add sector power to miner
 * @then power successfully added
 */
TEST(StoragePowerActor, addClaimedPowerForSector_Success) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  fc::primitives::BigInt min_candidate_storage_value =
      StoragePowerActor::kMinMinerSizeStor;

  fc::vm::actor::SectorStorageWeightDesc swd;
  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(min_candidate_storage_value));

  StoragePowerActor actor(indices, randomness_provider);

  Address addr{Network::MAINNET, 3232104785};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor.addClaimedPowerForSector(addr, swd));
  EXPECT_OUTCOME_EQ(actor.getClaimedPowerForMiner(addr),
                    min_candidate_storage_value);
  EXPECT_OUTCOME_EQ(actor.getNominalPowerForMiner(addr),
                    min_candidate_storage_value);
  EXPECT_OUTCOME_EQ(actor.getPowerTotalForMiner(addr),
                    min_candidate_storage_value);
}

/**
 * @given Storage Power Actor and sector
 * @when try to add sector power to miner, but less that need for consensus
 * @then power successfully added, but total power is 0
 */
TEST(StoragePowerActor,
     addClaimedPowerForSectorSuccess_ButLessThatMinCandidateStorage) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  fc::vm::actor::SectorStorageWeightDesc swd;
  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  StoragePowerActor actor(indices, randomness_provider);

  Address addr{Network::MAINNET, 3232104785};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor.addClaimedPowerForSector(addr, swd));
  EXPECT_OUTCOME_EQ(actor.getClaimedPowerForMiner(addr), 1);
  EXPECT_OUTCOME_EQ(actor.getNominalPowerForMiner(addr), 1);
  EXPECT_OUTCOME_EQ(actor.getPowerTotalForMiner(addr), 0);
}

/**
 * @given Storage Power Actor and sector
 * @when try to add sector power to miner, but miner fail proof of space time
 * @then power successfully added, but nominal and total power is 0
 */
TEST(StoragePowerActor, addClaimedPowerForSector_FailPoSt) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  fc::vm::actor::SectorStorageWeightDesc swd;
  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  StoragePowerActor actor(indices, randomness_provider);

  Address addr{Network::MAINNET, 3232104785};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor.addFaultMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor.addClaimedPowerForSector(addr, swd));
  EXPECT_OUTCOME_EQ(actor.getClaimedPowerForMiner(addr), 1);
  EXPECT_OUTCOME_EQ(actor.getNominalPowerForMiner(addr), 0);
  EXPECT_OUTCOME_EQ(actor.getPowerTotalForMiner(addr), 0);
}

/**
 * @given Storage Power Actor and sector
 * @when try to deduct sector power from miner
 * @then power successfully deducted
 */
TEST(StoragePowerActor, deductClaimedPowerForSectorAssert_Success) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  fc::vm::actor::SectorStorageWeightDesc swd;
  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  StoragePowerActor actor(indices, randomness_provider);

  Address addr{Network::MAINNET, 3232104785};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor.addClaimedPowerForSector(addr, swd));
  EXPECT_OUTCOME_TRUE_1(actor.deductClaimedPowerForSectorAssert(addr, swd));
  EXPECT_OUTCOME_EQ(actor.getClaimedPowerForMiner(addr), 0);
  EXPECT_OUTCOME_EQ(actor.getNominalPowerForMiner(addr), 0);
  EXPECT_OUTCOME_EQ(actor.getPowerTotalForMiner(addr), 0);
}

/**
 * @given Storage Power Actor and 3 miner and randomness
 * @when try to choose 2 miners
 * @then 2 miners successfully chosen
 */
TEST(StoragePowerActor, selectMinersToSurprise_Success) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  Randomness randomness;
  EXPECT_CALL(*randomness_provider, randomInt(randomness, _, _))
      .WillOnce(testing::Return(0))
      .WillOnce(testing::Return(2))
      .WillRepeatedly(testing::Return(1));

  StoragePowerActor actor(indices, randomness_provider);

  Address addr{Network::MAINNET, 3232104785};
  Address addr_1{Network::MAINNET, 323210478};
  Address addr_2{Network::MAINNET, 32321047};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr_1));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr_2));

  EXPECT_OUTCOME_TRUE(miners, actor.getMiners());

  EXPECT_OUTCOME_TRUE(sup_miners, actor.selectMinersToSurprise(2, randomness));

  ASSERT_THAT(sup_miners, testing::ElementsAre(miners[0], miners[2]));
}

/**
 * @given Storage Power Actor and 3 miner and randomness
 * @when try to choose 3 miners
 * @then all miners return
 */
TEST(StoragePowerActor, selectMinersToSurprise_All) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  Randomness randomness;
  EXPECT_CALL(*randomness_provider, randomInt(randomness, _, _))
      .WillRepeatedly(testing::Return(0));

  StoragePowerActor actor(indices, randomness_provider);

  Address addr{Network::MAINNET, 3232104785};
  Address addr_1{Network::MAINNET, 323210478};
  Address addr_2{Network::MAINNET, 32321047};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr_1));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr_2));

  EXPECT_OUTCOME_TRUE(miners, actor.getMiners());

  EXPECT_OUTCOME_TRUE(sup_miners,
                      actor.selectMinersToSurprise(miners.size(), randomness));

  ASSERT_THAT(sup_miners, miners);
}

/**
 * @given Storage Power Actor and 3 miner and randomness
 * @when try to choose more that 3 miners
 * @then OUT_OF_BOUND error
 */
TEST(StoragePowerActor, selectMinersToSurprise_MoreThatHave) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  Randomness randomness;
  EXPECT_CALL(*randomness_provider, randomInt(randomness, _, _))
      .WillRepeatedly(testing::Return(0));

  StoragePowerActor actor(indices, randomness_provider);

  Address addr{Network::MAINNET, 3232104785};
  Address addr_1{Network::MAINNET, 323210478};
  Address addr_2{Network::MAINNET, 32321047};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr_1));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addr_2));

  EXPECT_OUTCOME_TRUE(miners, actor.getMiners());

  EXPECT_OUTCOME_ERROR(
      StoragePowerActor::OUT_OF_BOUND,
      actor.selectMinersToSurprise(miners.size() + 1, randomness));
}
