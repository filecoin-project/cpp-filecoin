/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

#include <gtest/gtest.h>
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/parse.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_export.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::address::decodeFromString;
  using testing::Return;
  using testutil::vm::actor::builtin::ActorTestFixture;

  class MinerActorV0Test : public ActorTestFixture<State> {
   public:
    void SetUp() override {
      ActorTestFixture<State>::SetUp();
      anyCodeIdAddressIs(kAccountCodeCid);
    }

    void expectEnrollCronEvent(const ChainEpoch &proving_period_start) {
      CronEventPayload payload{CronEventType::kProvingDeadline};
      EXPECT_OUTCOME_TRUE(encoded_payload, codec::cbor::encode(payload));
      runtime.expectSendM<storage_power::EnrollCronEvent>(
          kStoragePowerAddress,
          {.event_epoch = proving_period_start - 1, .payload = encoded_payload},
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
  TEST_F(MinerActorV0Test, assignProvingPeriodOffset) {
    const Address address1 =
        decodeFromString("t2ssgkulnwdpcm3nh2652azver6gkqioiu2ez3zma").value();
    const Address address2 =
        decodeFromString("t2mzc3knjb7dvps7r5mqcdqwyygxnaxmjviyirqii").value();
    ChainEpoch epoch{1};

    EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(Return(address1));
    EXPECT_OUTCOME_EQ(Construct::assignProvingPeriodOffset(runtime, epoch),
                      ChainEpoch{863});

    EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(Return(address2));
    EXPECT_OUTCOME_EQ(Construct::assignProvingPeriodOffset(runtime, epoch),
                      ChainEpoch{1603});
  }

  /**
   * Test input data and result are generated from
   * TestAssignProvingPeriodBoundary in specs-actors 'miner_internal_test.go'
   */
  TEST_F(MinerActorV0Test, assignProvingPeriodOffsetFromFile) {
    const Address address =
        decodeFromString("t2ssgkulnwdpcm3nh2652azver6gkqioiu2ez3zma").value();
    EXPECT_CALL(runtime, getCurrentReceiver()).WillRepeatedly(Return(address));
    const auto test_data = parseCsvPair(resourcePath(
        "vm/actor/builtin/v0/miner/test_assign_proving_period_offset.txt"));
    for (const auto &p : test_data) {
      EXPECT_OUTCOME_EQ(Construct::assignProvingPeriodOffset(
                            runtime, p.first.convert_to<ChainEpoch>()),
                        ChainEpoch{p.second.convert_to<ChainEpoch>()});
    }
  }

  /**
   * Simple construction
   * @given vm
   * @when construct method called
   * @then empty miner actor created
   */
  TEST_F(MinerActorV0Test, SimpleConstruct) {
    callerIs(kInitAddress);
    currentEpochIs(0);

    const Address owner = Address::makeFromId(100);
    const Address worker = Address::makeFromId(101);
    expectAccountV0PubkeyAddressSend(worker, bls_pubkey);

    const std::vector<Address> control_addresses;
    const Buffer peer_id;
    const std::vector<Multiaddress> multiaddresses;

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));

    // This is just set from running the code.
    ChainEpoch proving_period_start{658};
    expectEnrollCronEvent(proving_period_start);

    EXPECT_OUTCOME_TRUE_1(Construct::call(
        runtime,
        Construct::Params{
            .owner = owner,
            .worker = worker,
            .control_addresses = control_addresses,
            .seal_proof_type = RegisteredSealProof::StackedDrg32GiBV1,
            .peer_id = peer_id,
            .multiaddresses = multiaddresses}));

    EXPECT_OUTCOME_TRUE(miner_info, ipld->getCbor<MinerInfo>(state.info));
    EXPECT_EQ(miner_info.owner, owner);
    EXPECT_EQ(miner_info.worker, worker);
    EXPECT_EQ(miner_info.control, control_addresses);
    EXPECT_EQ(miner_info.peer_id, peer_id);
    EXPECT_EQ(miner_info.multiaddrs, multiaddresses);
    EXPECT_EQ(static_cast<RegisteredSealProof>(miner_info.seal_proof_type),
              RegisteredSealProof::StackedDrg32GiBV1);
    EXPECT_EQ(miner_info.sector_size, BigInt{1} << 35);
    EXPECT_EQ(miner_info.window_post_partition_sectors, 2349);

    EXPECT_EQ(state.precommit_deposit, 0);
    EXPECT_EQ(state.locked_funds, 0);
    EXPECT_EQ(state.proving_period_start, proving_period_start);
    EXPECT_EQ(state.current_deadline, 0);

    EXPECT_OUTCOME_TRUE(deadlines, ipld->getCbor<Deadlines>(state.deadlines));
    EXPECT_EQ(deadlines.due.size(), kWPoStPeriodDeadlines);

    for (const auto &deadline_cid : deadlines.due) {
      EXPECT_OUTCOME_TRUE(deadline, ipld->getCbor<Deadline>(deadline_cid));
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
  TEST_F(MinerActorV0Test, ConstructResolvedControl) {
    callerIs(kInitAddress);
    currentEpochIs(0);

    const Address owner = Address::makeFromId(100);
    const Address worker = Address::makeFromId(101);
    expectAccountV0PubkeyAddressSend(worker, bls_pubkey);

    std::vector<Address> control_addresses;
    const Address control1 = Address::makeFromId(501);
    const Address controlId1 = Address::makeFromId(555);
    control_addresses.emplace_back(control1);
    resloveAddressAs(control1, controlId1);

    const Address control2 = Address::makeFromId(502);
    const Address controlId2 = Address::makeFromId(655);
    control_addresses.emplace_back(control2);
    resloveAddressAs(control2, controlId2);

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));

    // This is just set from running the code.
    ChainEpoch proving_period_start{658};
    expectEnrollCronEvent(proving_period_start);

    EXPECT_OUTCOME_TRUE_1(Construct::call(
        runtime,
        Construct::Params{
            .owner = owner,
            .worker = worker,
            .control_addresses = control_addresses,
            .seal_proof_type = RegisteredSealProof::StackedDrg32GiBV1,
            .peer_id = {},
            .multiaddresses = {}}));

    EXPECT_OUTCOME_TRUE(miner_info, ipld->getCbor<MinerInfo>(state.info));
    EXPECT_EQ(miner_info.control.size(), 2);
    EXPECT_EQ(miner_info.control[0], controlId1);
    EXPECT_EQ(miner_info.control[1], controlId2);
  }

  /**
   * @given vm and control addresses are not id addresses
   * @when miner constructor called
   * @then error returned
   */
  TEST_F(MinerActorV0Test, ConstructControlNotId) {
    callerIs(kInitAddress);
    currentEpochIs(0);

    const Address owner = Address::makeFromId(100);
    const Address worker = Address::makeFromId(101);
    expectAccountV0PubkeyAddressSend(worker, bls_pubkey);

    std::vector<Address> control_addresses;
    const Address control = Address::makeFromId(501);
    control_addresses.emplace_back(control);
    addressCodeIdIs(control, kCronCodeCid);

    EXPECT_OUTCOME_ERROR(
        VMAbortExitCode{VMExitCode::kErrIllegalArgument},
        Construct::call(
            runtime,
            Construct::Params{
                .owner = owner,
                .worker = worker,
                .control_addresses = control_addresses,
                .seal_proof_type = RegisteredSealProof::StackedDrg32GiBV1,
                .peer_id = {},
                .multiaddresses = {}}));
  }

}  // namespace fc::vm::actor::builtin::v0::miner
