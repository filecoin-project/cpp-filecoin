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

using fc::adt::Multimap;
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
  std::shared_ptr<Hamt> escrow_table = std::make_shared<Hamt>(datastore);
  std::shared_ptr<Multimap> cron_event_queue =
      std::make_shared<Multimap>(datastore);
  std::shared_ptr<Hamt> po_st_detected_fault_miners =
      std::make_shared<Hamt>(datastore);
  std::shared_ptr<Hamt> claims = std::make_shared<Hamt>(datastore);

  std::shared_ptr<StoragePowerActorState> actor_state =
      std::make_shared<StoragePowerActorState>(indices,
                                               randomness_provider,
                                               escrow_table,
                                               cron_event_queue,
                                               po_st_detected_fault_miners,
                                               claims);

  Address addr{Address::makeFromId(3232104785)};
  Address addr_1{Address::makeFromId(323210478)};
  Address addr_2{Address::makeFromId(32321047)};

  Claim default_claim{.power = 1, .pledge = 0};
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

  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr, default_claim));
  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr_1));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr_1, default_claim));
  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr_2));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr_2, default_claim));

  EXPECT_OUTCOME_TRUE(miners, actor_state->getMiners());

  EXPECT_OUTCOME_TRUE(sup_miners,
                      actor_state->selectMinersToSurprise(2, randomness));

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

  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr, default_claim));
  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr_1));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr_1, default_claim));
  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr_2));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr_2, default_claim));

  EXPECT_OUTCOME_TRUE(miners, actor_state->getMiners());
  ASSERT_EQ(miners.size(), 3);

  EXPECT_OUTCOME_TRUE(
      sup_miners,
      actor_state->selectMinersToSurprise(miners.size(), randomness));

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

  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr, default_claim));
  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr_1));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr_1, default_claim));
  EXPECT_OUTCOME_TRUE_1(actor_state->addMiner(addr_2));
  EXPECT_OUTCOME_TRUE_1(actor_state->setClaim(addr_2, default_claim));

  EXPECT_OUTCOME_TRUE(miners, actor_state->getMiners());

  EXPECT_OUTCOME_ERROR(
      VMExitCode::STORAGE_POWER_ACTOR_OUT_OF_BOUND,
      actor_state->selectMinersToSurprise(miners.size() + 1, randomness));
}
