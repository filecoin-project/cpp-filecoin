/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

#include "primitives/address/address.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/parse.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/vm/actor/builtin/miner/miner_actor_test_fixture.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor_utils.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor.hpp"
#include "vm/actor/codes.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using common::smoothing::FilterEstimate;
  using crypto::randomness::Randomness;
  using primitives::kChainEpochUndefined;
  using primitives::address::decodeFromString;
  using primitives::sector::RegisteredSealProof;
  using states::MinerActorStatePtr;
  using testing::_;
  using testing::Return;
  using testutil::vm::actor::builtin::miner::MinerActorTestFixture;
  using types::Universal;
  using namespace types::miner;

  class MinerActorTest : public MinerActorTestFixture {
   public:
    void SetUp() override {
      MinerActorTestFixture::SetUp();
      actor_version = ActorVersion::kVersion0;
      ipld->actor_version = actor_version;
      state = MinerActorStatePtr{actor_version};
      anyCodeIdAddressIs(kAccountCodeId);
      cbor_blake::cbLoadT(ipld, state);

      currentEpochIs(kUpgradeSmokeHeight + 1);
    }

    void expectEnrollCronEvent(ChainEpoch event_epoch,
                               CronEventType event_type) {
      CronEventPayload payload{event_type};
      EXPECT_OUTCOME_TRUE(encoded_payload, codec::cbor::encode(payload));
      runtime.expectSendM<storage_power::EnrollCronEvent>(
          kStoragePowerAddress,
          {.event_epoch = event_epoch, .payload = encoded_payload},
          0,
          {});
    }

    void expectThisEpochReward(const TokenAmount &epoch_reward,
                               const FilterEstimate &reward_smoothed,
                               const StoragePower &baseline_power) {
      runtime.expectSendM<reward::ThisEpochReward>(
          kRewardAddress,
          {},
          0,
          {epoch_reward, reward_smoothed, baseline_power});
    }

    void expectCurrentTotalPower(const StoragePower &raw,
                                 const StoragePower &qa,
                                 const TokenAmount &pledge_collateral,
                                 const FilterEstimate &qa_power_smoothed) {
      runtime.expectSendM<storage_power::CurrentTotalPower>(
          kStoragePowerAddress,
          {},
          0,
          {raw, qa, pledge_collateral, qa_power_smoothed});
    }

    const Blob<48> bls_pubkey =
        "1234567890123456789012345678901234567890"
        "1234567890123456789012345678901234567890"
        "1122334455667788"_blob48;
  };

  /**
   * Test input data and result are from TestAssignProvingPeriodBoundary in
   * specs-actors 'miner_internal_test.go'
   */
  TEST_F(MinerActorTest, assignProvingPeriodOffset) {
    const Address address1 =
        decodeFromString("t2ssgkulnwdpcm3nh2652azver6gkqioiu2ez3zma").value();
    const Address address2 =
        decodeFromString("t2mzc3knjb7dvps7r5mqcdqwyygxnaxmjviyirqii").value();
    ChainEpoch epoch{1};

    MinerUtils utils{runtime};

    EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(Return(address1));
    EXPECT_OUTCOME_EQ(utils.assignProvingPeriodOffset(epoch), ChainEpoch{863});

    EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(Return(address2));
    EXPECT_OUTCOME_EQ(utils.assignProvingPeriodOffset(epoch), ChainEpoch{1603});
  }

  /**
   * Test input data and result are generated from
   * TestAssignProvingPeriodBoundary in specs-actors 'miner_internal_test.go'
   */
  TEST_F(MinerActorTest, assignProvingPeriodOffsetFromFile) {
    const Address address =
        decodeFromString("t2ssgkulnwdpcm3nh2652azver6gkqioiu2ez3zma").value();
    EXPECT_CALL(runtime, getCurrentReceiver()).WillRepeatedly(Return(address));
    const auto test_data = parseCsvPair(resourcePath(
        "vm/actor/builtin/v0/miner/test_assign_proving_period_offset.txt"));

    MinerUtils utils{runtime};

    for (const auto &p : test_data) {
      EXPECT_OUTCOME_EQ(
          utils.assignProvingPeriodOffset(p.first.convert_to<ChainEpoch>()),
          ChainEpoch{p.second.convert_to<ChainEpoch>()});
    }
  }

  /**
   * Simple construction
   * @given vm
   * @when construct method called
   * @then empty miner actor created
   */
  TEST_F(MinerActorTest, SimpleConstruct) {
    callerIs(kInitAddress);

    expectAccountV0PubkeyAddressSend(worker, bls_pubkey);

    const std::vector<Address> control_addresses;
    const Buffer peer_id;
    const std::vector<Multiaddress> multiaddresses;

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));

    // This is just set from running the code.
    const ChainEpoch proving_period_start{53870};
    expectEnrollCronEvent(proving_period_start - 1,
                          CronEventType::kProvingDeadline);

    EXPECT_OUTCOME_TRUE_1(Construct::call(
        runtime,
        Construct::Params{
            .owner = owner,
            .worker = worker,
            .control_addresses = control_addresses,
            .seal_proof_type = RegisteredSealProof::kStackedDrg32GiBV1,
            .peer_id = peer_id,
            .multiaddresses = multiaddresses}));

    EXPECT_OUTCOME_TRUE(miner_info, state->getInfo());
    EXPECT_EQ(miner_info->owner, owner);
    EXPECT_EQ(miner_info->worker, worker);
    EXPECT_EQ(miner_info->control, control_addresses);
    EXPECT_EQ(miner_info->peer_id, peer_id);
    EXPECT_EQ(miner_info->multiaddrs, multiaddresses);
    EXPECT_EQ(static_cast<RegisteredSealProof>(miner_info->seal_proof_type),
              RegisteredSealProof::kStackedDrg32GiBV1);
    EXPECT_EQ(miner_info->sector_size, BigInt{1} << 35);
    EXPECT_EQ(miner_info->window_post_partition_sectors, 2349);

    EXPECT_EQ(state->precommit_deposit, 0);
    EXPECT_EQ(state->locked_funds, 0);
    EXPECT_EQ(state->proving_period_start, proving_period_start);
    EXPECT_EQ(state->current_deadline, 0);

    EXPECT_OUTCOME_TRUE(deadlines, state->deadlines.get());
    EXPECT_EQ(deadlines.due.size(), kWPoStPeriodDeadlines);

    for (const auto &deadline_cid : deadlines.due) {
      EXPECT_OUTCOME_TRUE(deadline, deadline_cid.get());
      EXPECT_OUTCOME_EQ(deadline->partitions.size(), 0);
      EXPECT_OUTCOME_EQ(deadline->expirations_epochs.size(), 0);
      EXPECT_TRUE(deadline->partitions_posted.empty());
      EXPECT_TRUE(deadline->early_terminations.empty());
      EXPECT_EQ(deadline->live_sectors, 0);
      EXPECT_EQ(deadline->total_sectors, 0);
      EXPECT_EQ(deadline->faulty_power.raw, 0);
      EXPECT_EQ(deadline->faulty_power.qa, 0);
    }
  }

  /**
   * @given vm and control addresses are resolvable
   * @when miner is constructed
   * @then control addresses are resolved
   */
  TEST_F(MinerActorTest, ConstructResolvedControl) {
    callerIs(kInitAddress);

    expectAccountV0PubkeyAddressSend(worker, bls_pubkey);

    std::vector<Address> control_addresses;
    const Address control1 = Address::makeFromId(501);
    const Address controlId1 = Address::makeFromId(555);
    control_addresses.emplace_back(control1);
    resolveAddressAs(control1, controlId1);

    const Address control2 = Address::makeFromId(502);
    const Address controlId2 = Address::makeFromId(655);
    control_addresses.emplace_back(control2);
    resolveAddressAs(control2, controlId2);

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));

    // This is just set from running the code.
    const ChainEpoch proving_period_start{53870};
    expectEnrollCronEvent(proving_period_start - 1,
                          CronEventType::kProvingDeadline);

    EXPECT_OUTCOME_TRUE_1(Construct::call(
        runtime,
        Construct::Params{
            .owner = owner,
            .worker = worker,
            .control_addresses = control_addresses,
            .seal_proof_type = RegisteredSealProof::kStackedDrg32GiBV1,
            .peer_id = {},
            .multiaddresses = {}}));

    EXPECT_OUTCOME_TRUE(miner_info, state->getInfo());
    EXPECT_EQ(miner_info->control.size(), 2);
    EXPECT_EQ(miner_info->control[0], controlId1);
    EXPECT_EQ(miner_info->control[1], controlId2);
  }

  /**
   * @given vm and control addresses are not id addresses
   * @when miner constructor called
   * @then error returned
   */
  TEST_F(MinerActorTest, ConstructControlNotId) {
    callerIs(kInitAddress);

    const Address owner = Address::makeFromId(100);
    const Address worker = Address::makeFromId(101);
    expectAccountV0PubkeyAddressSend(worker, bls_pubkey);

    std::vector<Address> control_addresses;
    control_addresses.emplace_back(control);
    addressCodeIdIs(control, kCronCodeId);

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        Construct::call(
            runtime,
            Construct::Params{
                .owner = owner,
                .worker = worker,
                .control_addresses = control_addresses,
                .seal_proof_type = RegisteredSealProof::kStackedDrg32GiBV1,
                .peer_id = {},
                .multiaddresses = {}}));
  }

  /**
   * @given state is created
   * @when miner ControlAddresses called
   * @then properly values are returned
   */
  TEST_F(MinerActorTest, ControlAddressesSuccess) {
    initEmptyState();
    initDefaultMinerInfo();

    EXPECT_OUTCOME_TRUE(result, ControlAddresses::call(runtime, {}));

    EXPECT_EQ(result.owner, owner);
    EXPECT_EQ(result.worker, worker);
    EXPECT_EQ(result.control.size(), 1);
    EXPECT_EQ(result.control[0], control);
  }

  /**
   * @given caller is not owner
   * @when miner ChangeWorkerAddress called
   * @then kSysErrForbidden returned
   */
  TEST_F(MinerActorTest, ChangeWorkerAddressWrongCaller) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(kInitAddress);

    const Address new_worker = Address::makeFromId(201);
    expectAccountV0PubkeyAddressSend(new_worker, bls_pubkey);

    std::vector<Address> new_control_addresses;
    const Address control1 = Address::makeFromId(701);
    const Address controlId1 = Address::makeFromId(751);
    new_control_addresses.emplace_back(control1);
    resolveAddressAs(control1, controlId1);

    const Address control2 = Address::makeFromId(702);
    const Address controlId2 = Address::makeFromId(752);
    new_control_addresses.emplace_back(control2);
    resolveAddressAs(control2, controlId2);

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kSysErrForbidden),
        ChangeWorkerAddress::call(
            runtime,
            ChangeWorkerAddress::Params{new_worker, new_control_addresses}));
  }

  /**
   * @given vm
   * @when miner ChangeWorkerAddress called
   * @then new worker is recorded to pending_worker_key
   */
  TEST_F(MinerActorTest, ChangeWorkerAddressSuccess) {
    initEmptyState();
    initDefaultMinerInfo();

    const ChainEpoch effective_epoch{current_epoch + kWorkerKeyChangeDelay};

    callerIs(owner);

    const Address new_worker = Address::makeFromId(201);
    expectAccountV0PubkeyAddressSend(new_worker, bls_pubkey);

    std::vector<Address> new_control_addresses;
    const Address control1 = Address::makeFromId(701);
    const Address controlId1 = Address::makeFromId(751);
    new_control_addresses.emplace_back(control1);
    resolveAddressAs(control1, controlId1);

    const Address control2 = Address::makeFromId(702);
    const Address controlId2 = Address::makeFromId(752);
    new_control_addresses.emplace_back(control2);
    resolveAddressAs(control2, controlId2);

    expectEnrollCronEvent(effective_epoch, CronEventType::kWorkerKeyChange);

    EXPECT_OUTCOME_TRUE_1(ChangeWorkerAddress::call(
        runtime,
        ChangeWorkerAddress::Params{new_worker, new_control_addresses}));

    EXPECT_OUTCOME_TRUE(miner_info, state->getInfo());
    EXPECT_EQ(miner_info->pending_worker_key.get().new_worker, new_worker);
    EXPECT_EQ(miner_info->pending_worker_key.get().effective_at,
              effective_epoch);
    EXPECT_EQ(miner_info->control.size(), 2);
    EXPECT_EQ(miner_info->control[0], controlId1);
    EXPECT_EQ(miner_info->control[1], controlId2);
  }

  /**
   * @given caller is not owner, worker or control address
   * @when miner ChangePeerId called
   * @then kSysErrForbidden returned
   */
  TEST_F(MinerActorTest, ChangePeerIdWrongCaller) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(kInitAddress);

    const Buffer new_peer_id{"0102"_unhex};

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kSysErrForbidden),
        ChangePeerId::call(runtime, ChangePeerId::Params{new_peer_id}));
  }

  /**
   * @given vm
   * @when miner ChangePeerId called
   * @then new peer id is recorded to miner info
   */
  TEST_F(MinerActorTest, ChangePeerIdSuccess) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(owner);

    const Buffer new_peer_id{"0102"_unhex};

    EXPECT_OUTCOME_TRUE_1(
        ChangePeerId::call(runtime, ChangePeerId::Params{new_peer_id}));

    EXPECT_OUTCOME_TRUE(miner_info, state->getInfo());
    EXPECT_EQ(miner_info->peer_id, new_peer_id);
  }

  TEST_F(MinerActorTest, SubmitWindowedPoStWrongParams) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(owner);

    const uint64_t expected_deadline_id = 1;
    const uint64_t wrong_deadline_id = 3;

    state->current_deadline = expected_deadline_id;
    state->proving_period_start =
        current_epoch - 10 - expected_deadline_id * kWPoStChallengeWindow;

    const ChainEpoch chain_commit_epoch = current_epoch - 10;

    EXPECT_OUTCOME_TRUE(
        expected_randomness,
        Randomness::fromString("i_am_random_____i_am_random_____"));

    EXPECT_OUTCOME_TRUE(
        wrong_randomness,
        Randomness::fromString("wrong_random____wrong_random____"));

    EXPECT_CALL(runtime, getRandomnessFromTickets(_, _, _))
        .WillRepeatedly(Return(expected_randomness));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{.deadline = kWPoStPeriodDeadlines,
                                       .partitions = {},
                                       .proofs = {},
                                       .chain_commit_epoch = {},
                                       .chain_commit_rand = {}}));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{.deadline = expected_deadline_id,
                                       .partitions = {},
                                       .proofs = {},
                                       .chain_commit_epoch = current_epoch + 1,
                                       .chain_commit_rand = {}}));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{
                .deadline = expected_deadline_id,
                .partitions = {},
                .proofs = {},
                .chain_commit_epoch = current_epoch - kWPoStChallengeWindow - 1,
                .chain_commit_rand = {}}));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{.deadline = expected_deadline_id,
                                       .partitions = {},
                                       .proofs = {},
                                       .chain_commit_epoch = chain_commit_epoch,
                                       .chain_commit_rand = wrong_randomness}));

    expectThisEpochReward(100, {10, 10}, 10);
    expectCurrentTotalPower(100, 100, 1000, {10, 10});

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         SubmitWindowedPoSt::call(
                             runtime,
                             SubmitWindowedPoSt::Params{
                                 .deadline = expected_deadline_id,
                                 .partitions = {5, PoStPartition()},
                                 .proofs = {},
                                 .chain_commit_epoch = chain_commit_epoch,
                                 .chain_commit_rand = expected_randomness}));

    expectThisEpochReward(100, {10, 10}, 10);
    expectCurrentTotalPower(100, 100, 1000, {10, 10});

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         SubmitWindowedPoSt::call(
                             runtime,
                             SubmitWindowedPoSt::Params{
                                 .deadline = wrong_deadline_id,
                                 .partitions = {},
                                 .proofs = {},
                                 .chain_commit_epoch = chain_commit_epoch,
                                 .chain_commit_rand = expected_randomness}));
  }

  TEST_F(MinerActorTest, SubmitWindowedPoStSuccess) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(owner);
    balance = 1000;

    const uint64_t deadline_id = 1;
    const uint64_t partition_id = 0;

    state->current_deadline = deadline_id;
    state->proving_period_start =
        current_epoch - 10 - deadline_id * kWPoStChallengeWindow;

    const ChainEpoch chain_commit_epoch = current_epoch - 10;

    std::vector<SectorOnChainInfo> sectors;
    for (uint64_t i = 0; i < 4; i++) {
      SectorOnChainInfo sector;
      sector.sector = i;
      sector.sealed_cid = kEmptyObjectCid;
      sectors.push_back(sector);
    }

    EXPECT_OUTCOME_TRUE_1(state->sectors.store(sectors));

    Universal<Partition> partition{actor_version};
    cbor_blake::cbLoadT(ipld, partition);
    partition->sectors = {0, 1, 2, 3};
    partition->faults = {2, 3};

    Universal<Deadline> deadline{actor_version};
    cbor_blake::cbLoadT(ipld, deadline);
    EXPECT_OUTCOME_TRUE_1(deadline->partitions.set(partition_id, partition));

    EXPECT_OUTCOME_TRUE(deadlines, state->deadlines.get());
    EXPECT_OUTCOME_TRUE_1(deadlines.due[deadline_id].set(deadline));
    EXPECT_OUTCOME_TRUE_1(state->deadlines.set(deadlines));

    const PoStProof post_proof{
        .registered_proof = RegisteredPoStProof::kStackedDRG32GiBWindowPoSt,
        .proof = {}};

    EXPECT_OUTCOME_TRUE(
        randomness, Randomness::fromString("i_am_random_____i_am_random_____"));

    EXPECT_CALL(runtime, getRandomnessFromTickets(_, _, _))
        .WillRepeatedly(Return(randomness));

    EXPECT_CALL(runtime, getRandomnessFromBeacon(_, _, _))
        .WillOnce(Return(randomness));

    EXPECT_CALL(runtime, verifyPoSt(_)).WillOnce(Return(true));

    expectThisEpochReward(100, {10, 10}, 10);
    expectCurrentTotalPower(100, 100, 1000, {10, 10});

    EXPECT_OUTCOME_TRUE_1(SubmitWindowedPoSt::call(
        runtime,
        SubmitWindowedPoSt::Params{
            .deadline = deadline_id,
            .partitions = {{.index = partition_id, .skipped = {}}},
            .proofs = {post_proof},
            .chain_commit_epoch = chain_commit_epoch,
            .chain_commit_rand = randomness}));
  }

}  // namespace fc::vm::actor::builtin::v0::miner
