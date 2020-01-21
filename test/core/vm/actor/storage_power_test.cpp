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
TEST(StoragePowerActor, AddMinerSuccess) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  StoragePowerActor actor(indices, randomness_provider);

  Address addrID_0{Network::MAINNET, 3232104785};
  EXPECT_OUTCOME_FALSE(err, actor.getPowerTotalForMiner(addrID_0));
  ASSERT_EQ(err, PowerTableError::NO_SUCH_MINER);
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_0));
  EXPECT_OUTCOME_TRUE(res, actor.getPowerTotalForMiner(addrID_0));
  ASSERT_EQ(res, 0);
}

/**
 * @given Storage Power Actor and 1 miner
 * @when try to add same miner again
 * @then error ALREADY_EXIST
 */
TEST(StoragePowerActor, AddMinerTwice) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  StoragePowerActor actor(indices, randomness_provider);

  Address addrID_0{Network::MAINNET, 3232104785};
  EXPECT_OUTCOME_FALSE(err, actor.getPowerTotalForMiner(addrID_0));
  ASSERT_EQ(err, PowerTableError::NO_SUCH_MINER);
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_0));
  EXPECT_OUTCOME_FALSE(err1, actor.addMiner(addrID_0));
  ASSERT_EQ(err1, StoragePowerActor::ALREADY_EXIST);
}

/**
 * @given Storage Power Actor and 1 miner
 * @when try to remove the miner
 * @then miner successfully removed
 */
TEST(StoragePowerActor, RemoveMinerSuccess) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  StoragePowerActor actor(indices, randomness_provider);

  Address addrID_0{Network::MAINNET, 3232104785};
  EXPECT_OUTCOME_FALSE(err, actor.getPowerTotalForMiner(addrID_0));
  ASSERT_EQ(err, PowerTableError::NO_SUCH_MINER);
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_0));
  EXPECT_OUTCOME_TRUE(res, actor.getPowerTotalForMiner(addrID_0));
  ASSERT_EQ(res, 0);
  EXPECT_OUTCOME_TRUE_1(actor.removeMiner(addrID_0));
  EXPECT_OUTCOME_FALSE(err1, actor.getPowerTotalForMiner(addrID_0));
  ASSERT_EQ(err1, PowerTableError::NO_SUCH_MINER);
}

/**
 * @given Storage Power Actor
 * @when try to remove no existen miner
 * @then error NO_SUCH_MINER
 */
TEST(StoragePowerActor, RemoveMinerNoMiner) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  StoragePowerActor actor(indices, randomness_provider);

  Address addrID_0{Network::MAINNET, 3232104785};
  EXPECT_OUTCOME_FALSE(err, actor.removeMiner(addrID_0));
  ASSERT_EQ(err, PowerTableError::NO_SUCH_MINER);
}

/**
 * @given Storage Power Actor and sector
 * @when try to add sector power to miner
 * @then power successfully added
 */
TEST(StoragePowerActor, addClaimedPowerForSectorSuccess) {
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

  Address addrID_0{Network::MAINNET, 3232104785};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_0));
  EXPECT_OUTCOME_TRUE_1(actor.addClaimedPowerForSector(addrID_0, swd));
  EXPECT_OUTCOME_EQ(actor.getClaimedPowerForMiner(addrID_0),
                    min_candidate_storage_value);
  EXPECT_OUTCOME_EQ(actor.getNominalPowerForMiner(addrID_0),
                    min_candidate_storage_value);
  EXPECT_OUTCOME_EQ(actor.getPowerTotalForMiner(addrID_0),
                    min_candidate_storage_value);
}

/**
 * @given Storage Power Actor and sector
 * @when try to add sector power to miner, but less that need for consensus
 * @then power successfully added, but total power is 0
 */
TEST(StoragePowerActor,
     addClaimedPowerForSectorSuccessButLessThatMinCandidateStorage) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  fc::vm::actor::SectorStorageWeightDesc swd;
  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  StoragePowerActor actor(indices, randomness_provider);

  Address addrID_0{Network::MAINNET, 3232104785};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_0));
  EXPECT_OUTCOME_TRUE_1(actor.addClaimedPowerForSector(addrID_0, swd));
  EXPECT_OUTCOME_EQ(actor.getClaimedPowerForMiner(addrID_0), 1);
  EXPECT_OUTCOME_EQ(actor.getNominalPowerForMiner(addrID_0), 1);
  EXPECT_OUTCOME_EQ(actor.getPowerTotalForMiner(addrID_0), 0);
}

/**
 * @given Storage Power Actor and sector
 * @when try to add sector power to miner, but miner fail proof of space time
 * @then power successfully added, but nominal and total power is 0
 */
TEST(StoragePowerActor, addClaimedPowerForSectorFailPoSt) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  fc::vm::actor::SectorStorageWeightDesc swd;
  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  StoragePowerActor actor(indices, randomness_provider);

  Address addrID_0{Network::MAINNET, 3232104785};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_0));
  EXPECT_OUTCOME_TRUE_1(actor.addFaultMiner(addrID_0));
  EXPECT_OUTCOME_TRUE_1(actor.addClaimedPowerForSector(addrID_0, swd));
  EXPECT_OUTCOME_EQ(actor.getClaimedPowerForMiner(addrID_0), 1);
  EXPECT_OUTCOME_EQ(actor.getNominalPowerForMiner(addrID_0), 0);
  EXPECT_OUTCOME_EQ(actor.getPowerTotalForMiner(addrID_0), 0);
}

/**
 * @given Storage Power Actor and sector
 * @when try to deduct sector power from miner
 * @then power successfully deducted
 */
TEST(StoragePowerActor, deductClaimedPowerForSectorAssertSuccess) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  fc::vm::actor::SectorStorageWeightDesc swd;
  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  StoragePowerActor actor(indices, randomness_provider);

  Address addrID_0{Network::MAINNET, 3232104785};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_0));
  EXPECT_OUTCOME_TRUE_1(actor.addClaimedPowerForSector(addrID_0, swd));
  EXPECT_OUTCOME_TRUE_1(actor.deductClaimedPowerForSectorAssert(addrID_0, swd));
  EXPECT_OUTCOME_EQ(actor.getClaimedPowerForMiner(addrID_0), 0);
  EXPECT_OUTCOME_EQ(actor.getNominalPowerForMiner(addrID_0), 0);
  EXPECT_OUTCOME_EQ(actor.getPowerTotalForMiner(addrID_0), 0);
}

/**
 * @given Storage Power Actor and 3 miner and randomness
 * @when try to choose 2 miners
 * @then 2 miners successfully chosen
 */
TEST(StoragePowerActor, selectMinersToSurpriseSuccess) {
  std::shared_ptr<MockIndices> indices = std::make_shared<MockIndices>();
  std::shared_ptr<MockRandomnessProvider> randomness_provider =
      std::make_shared<MockRandomnessProvider>();

  EXPECT_CALL(*indices, storagePowerConsensusMinMinerPower())
      .WillRepeatedly(testing::Return(1));

  EXPECT_CALL(*indices, consensusPowerForStorageWeight(_))
      .WillRepeatedly(testing::Return(1));

  Randomness randomness;
  EXPECT_CALL(*randomness_provider, randomInt(randomness, _, _))
      .WillRepeatedly(testing::Return(1));

  StoragePowerActor actor(indices, randomness_provider);

  Address addrID_0{Network::MAINNET, 3232104785};
  Address addrID_1{Network::MAINNET, 323210478};
  Address addrID_2{Network::MAINNET, 32321047};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_0));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_1));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_2));

  EXPECT_OUTCOME_TRUE(miners, actor.getMiners());

  EXPECT_OUTCOME_TRUE(sup_miners, actor.selectMinersToSurprise(2, randomness));

  ASSERT_THAT(sup_miners, testing::ElementsAre(miners[1], miners[2]));
}

/**
 * @given Storage Power Actor and 3 miner and randomness
 * @when try to choose 3 miners
 * @then all miners return
 */
TEST(StoragePowerActor, selectMinersToSurpriseAll) {
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

  Address addrID_0{Network::MAINNET, 3232104785};
  Address addrID_1{Network::MAINNET, 323210478};
  Address addrID_2{Network::MAINNET, 32321047};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_0));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_1));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_2));

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
TEST(StoragePowerActor, selectMinersToSurpriseMoreThatHave) {
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

  Address addrID_0{Network::MAINNET, 3232104785};
  Address addrID_1{Network::MAINNET, 323210478};
  Address addrID_2{Network::MAINNET, 32321047};

  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_0));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_1));
  EXPECT_OUTCOME_TRUE_1(actor.addMiner(addrID_2));

  EXPECT_OUTCOME_TRUE(miners, actor.getMiners());

  EXPECT_OUTCOME_FALSE(
      err, actor.selectMinersToSurprise(miners.size() + 1, randomness));

  ASSERT_EQ(err, StoragePowerActor::OUT_OF_BOUND);
}
