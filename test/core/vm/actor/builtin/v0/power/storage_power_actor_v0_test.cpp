/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/storage_power/storage_power_actor_export.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
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

      EXPECT_CALL(runtime, commit(testing::_))
          .Times(testing::AtMost(1))
          .WillOnce(testing::Invoke([&](auto &cid) {
            EXPECT_OUTCOME_TRUE(new_state, ipld->getCbor<State>(cid));
            state = std::move(new_state);
            return fc::outcome::success();
          }));
    }

   public:
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
    EXPECT_EQ(state.this_epoch_qa_power_smoothed.position,
              BigInt("274031556999544297163190906134303066185487351808000000"));
    EXPECT_EQ(state.this_epoch_qa_power_smoothed.velocity,
              BigInt("1403041571837666801475537439407631698869695241256960"));
  }

}  // namespace fc::vm::actor::builtin::v0::storage_power
