/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

#include "primitives/address/address.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/parse.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/vm/actor/builtin/miner/miner_actor_test_fixture.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/types/miner/v0/deadline.hpp"
#include "vm/actor/builtin/states/miner/v0/miner_actor_state.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor_utils.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_export.hpp"
#include "vm/actor/codes.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::kChainEpochUndefined;
  using primitives::address::decodeFromString;
  using primitives::sector::RegisteredSealProof;
  using testing::Return;
  using testutil::vm::actor::builtin::miner::MinerActorTestFixture;
  using namespace types::miner;

  class MinerActorTest : public MinerActorTestFixture<MinerActorState> {
   public:
    void SetUp() override {
      MinerActorTestFixture<MinerActorState>::SetUp();
      actor_version = ActorVersion::kVersion0;
      ipld->actor_version = actor_version;
      anyCodeIdAddressIs(kAccountCodeId);
      cbor_blake::cbLoadT(ipld, state);
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
    currentEpochIs(0);

    expectAccountV0PubkeyAddressSend(worker, bls_pubkey);

    const std::vector<Address> control_addresses;
    const Buffer peer_id;
    const std::vector<Multiaddress> multiaddresses;

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));

    // This is just set from running the code.
    const ChainEpoch proving_period_start{658};
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

    EXPECT_OUTCOME_TRUE(miner_info, state.getInfo());
    EXPECT_EQ(miner_info->owner, owner);
    EXPECT_EQ(miner_info->worker, worker);
    EXPECT_EQ(miner_info->control, control_addresses);
    EXPECT_EQ(miner_info->peer_id, peer_id);
    EXPECT_EQ(miner_info->multiaddrs, multiaddresses);
    EXPECT_EQ(static_cast<RegisteredSealProof>(miner_info->seal_proof_type),
              RegisteredSealProof::kStackedDrg32GiBV1);
    EXPECT_EQ(miner_info->sector_size, BigInt{1} << 35);
    EXPECT_EQ(miner_info->window_post_partition_sectors, 2349);

    EXPECT_EQ(state.precommit_deposit, 0);
    EXPECT_EQ(state.locked_funds, 0);
    EXPECT_EQ(state.proving_period_start, proving_period_start);
    EXPECT_EQ(state.current_deadline, 0);

    EXPECT_OUTCOME_TRUE(deadlines, state.deadlines.get());
    EXPECT_EQ(deadlines.due.size(), kWPoStPeriodDeadlines);

    for (const auto &deadline_cid : deadlines.due) {
      EXPECT_OUTCOME_TRUE(deadline, getCbor<Deadline>(ipld, deadline_cid));
      EXPECT_OUTCOME_EQ(deadline.partitions.size(), 0);
      EXPECT_OUTCOME_EQ(deadline.expirations_epochs.size(), 0);
      EXPECT_TRUE(deadline.post_submissions.empty());
      EXPECT_TRUE(deadline.early_terminations.empty());
      EXPECT_EQ(deadline.live_sectors, 0);
      EXPECT_EQ(deadline.total_sectors, 0);
      EXPECT_EQ(deadline.faulty_power.raw, 0);
      EXPECT_EQ(deadline.faulty_power.qa, 0);
    }
  }

  /**
   * @given vm and control addresses are resolvable
   * @when miner is constructed
   * @then control addresses are resolved
   */
  TEST_F(MinerActorTest, ConstructResolvedControl) {
    callerIs(kInitAddress);
    currentEpochIs(0);

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
    const ChainEpoch proving_period_start{658};
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

    EXPECT_OUTCOME_TRUE(miner_info, state.getInfo());
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
    currentEpochIs(0);

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

    currentEpochIs(10);
    const ChainEpoch effective_epoch{10 + kWorkerKeyChangeDelay};

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

    EXPECT_OUTCOME_TRUE(miner_info, state.getInfo());
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

    EXPECT_OUTCOME_TRUE(miner_info, state.getInfo());
    EXPECT_EQ(miner_info->peer_id, new_peer_id);
  }

}  // namespace fc::vm::actor::builtin::v0::miner
