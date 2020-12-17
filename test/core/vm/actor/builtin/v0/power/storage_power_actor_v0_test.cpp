/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/storage_power/storage_power_actor_export.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"

#include <gtest/gtest.h>
#include <libp2p/peer/peer_id.hpp>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v0/init/init_actor.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
  using libp2p::multi::Multihash;
  using libp2p::peer::PeerId;
  using primitives::kChainEpochUndefined;
  using runtime::MockRuntime;
  using storage::ipfs::InMemoryDatastore;
  using testing::Return;

  class StoragePowerActorTest : public testing::Test {
    void SetUp() override {
      EXPECT_CALL(runtime, getIpfsDatastore())
          .Times(testing::AnyNumber())
          .WillRepeatedly(Return(ipld));

      EXPECT_CALL(runtime, getImmediateCaller())
          .Times(testing::AnyNumber())
          .WillRepeatedly(testing::Invoke([&]() { return caller; }));

      EXPECT_CALL(runtime, getCurrentActorState())
          .Times(testing::AnyNumber())
          .WillRepeatedly(testing::Invoke([&]() {
            EXPECT_OUTCOME_TRUE(cid, ipld->setCbor(state));
            return std::move(cid);
          }));

      EXPECT_CALL(runtime, commit(testing::_))
          .Times(testing::AnyNumber())
          .WillRepeatedly(testing::Invoke([&](auto &cid) {
            EXPECT_OUTCOME_TRUE(new_state, ipld->getCbor<State>(cid));
            state = std::move(new_state);
            return fc::outcome::success();
          }));
    }

   public:
    /**
     * Storage power state is initialized
     */
    void constructed() {
      caller = kSystemActorAddress;
      EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, {}));
    }

    /**
     * Ensures that caller has code id
     * @param code_id - expected code id
     */
    void callerCodeIdIs(const CID &code_id) {
      caller = Address::makeFromId(111);
      EXPECT_CALL(runtime, getActorCodeID(caller)).WillOnce(Return(code_id));
    }

    /**
     * Creates params for method CreateMiner
     * @return valid CreateMiner parameters
     */
    CreateMiner::Result createMiner(const Address &owner,
                                    const Address &worker,
                                    const Address &id_address,
                                    const Address &robust_address) {
      Buffer peer_id = Buffer(10, 'x');
      CreateMiner::Params params{
          .owner = owner,
          .worker = worker,
          .seal_proof_type = RegisteredProof::StackedDRG2KiBSeal,
          .peer_id = peer_id,
          .multiaddresses = {}};
      EXPECT_OUTCOME_TRUE(miner_params,
                          encodeActorParams(miner::Construct::Params{
                              .owner = params.owner,
                              .worker = params.worker,
                              .control_addresses = {},
                              .seal_proof_type = params.seal_proof_type,
                              .peer_id = params.peer_id,
                              .multiaddresses = params.multiaddresses}));
      runtime.expectSendM<init::Exec>(kInitAddress,
                                      {kStorageMinerCodeCid, miner_params},
                                      0,
                                      {id_address, robust_address});
      EXPECT_CALL(runtime, getValueReceived()).WillOnce(Return(TokenAmount{0}));

      EXPECT_OUTCOME_TRUE(result, CreateMiner::call(runtime, params));
      return result;
    }

    MockRuntime runtime;
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    Address caller;
    State state;
  };

  /**
   * @given caller is system actor
   * @when Costruction is called
   * @then state is saved and default values are correct
   */
  TEST_F(StoragePowerActorTest, SimpleConstruction) {
    caller = kSystemActorAddress;

    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, {}));

    // values from lotus specs-actors v0.9
    EXPECT_EQ(state.total_raw_power, StoragePower{0});
    EXPECT_EQ(state.total_raw_commited, StoragePower{0});
    EXPECT_EQ(state.total_qa_power, StoragePower{0});
    EXPECT_EQ(state.total_qa_commited, StoragePower{0});
    EXPECT_EQ(state.total_pledge, TokenAmount{0});
    EXPECT_EQ(state.this_epoch_raw_power, StoragePower{0});
    EXPECT_EQ(state.this_epoch_qa_power, StoragePower{0});
    EXPECT_EQ(state.this_epoch_pledge, TokenAmount{0});
    EXPECT_EQ(state.this_epoch_qa_power_smoothed.position,
              BigInt("274031556999544297163190906134303066185487351808000000"));
    EXPECT_EQ(state.this_epoch_qa_power_smoothed.velocity,
              BigInt("1403041571837666801475537439407631698869695241256960"));
    EXPECT_EQ(state.miner_count, 0);
    EXPECT_EQ(state.num_miners_meeting_min_power, 0);
    EXPECT_EQ(state.first_cron_epoch, ChainEpoch{0});
    EXPECT_EQ(state.last_processed_cron_epoch, kChainEpochUndefined);
    EXPECT_OUTCOME_EQ(state.claims.size(), 0);
    EXPECT_OUTCOME_EQ(state.cron_event_queue.size(), 0);
  }

  /**
   * @given storage power actor initialized and empty
   * @when create miner called
   * @then new miner is created
   */
  TEST_F(StoragePowerActorTest, CreateMiner) {
    constructed();
    callerCodeIdIs(kAccountCodeCid);

    Address owner = Address::makeFromId(101);
    Address worker = Address::makeFromId(103);
    Address id_address = Address::makeFromId(1001);
    Address robust_address = Address::makeFromId(1003);

    auto res = createMiner(owner, worker, id_address, robust_address);

    EXPECT_EQ(state.miner_count, 1);
    EXPECT_OUTCOME_TRUE(claim, state.claims.get(id_address));
    EXPECT_EQ(claim.raw_power, StoragePower{0});
    EXPECT_EQ(claim.qa_power, StoragePower{0});
    EXPECT_EQ(res.id_address, id_address);
    EXPECT_EQ(res.robust_address, robust_address);
  }

  /**
   * @given storage power actor initialized and empty and caller is not signable
   * @when create miner called
   * @then error kSysErrForbidden returned
   */
  TEST_F(StoragePowerActorTest, CreateMinerCallerNotSignable) {
    constructed();
    callerCodeIdIs(kEmptyObjectCid);

    EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrForbidden,
                         CreateMiner::call(runtime, {}));
  }

  /**
   * @given storage power actor initialized and empty and caller is not miner
   * @when UpdateClaimedPower called
   * @then error kSysErrForbidden returned
   */
  TEST_F(StoragePowerActorTest, UpdateClaimedPowerCallerNotMiner) {
    constructed();
    callerCodeIdIs(kEmptyObjectCid);

    UpdateClaimedPower::Params params{};
    EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrForbidden,
                         UpdateClaimedPower::call(runtime, params));
  }

  /**
   * @given storage power actor initialized and empty and caller is miner
   * @when UpdateClaimedPower called
   * @then error kErrNotFound returned
   */
  TEST_F(StoragePowerActorTest, UpdateClaimedPowerMinerNotFound) {
    constructed();
    callerCodeIdIs(kStorageMinerCodeCid);

    UpdateClaimedPower::Params params{};
    EXPECT_OUTCOME_ERROR(VMExitCode::kErrNotFound,
                         UpdateClaimedPower::call(runtime, params));
  }

}  // namespace fc::vm::actor::builtin::v0::storage_power
