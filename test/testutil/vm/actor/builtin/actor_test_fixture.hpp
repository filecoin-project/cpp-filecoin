/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>
#include "primitives/address/address.hpp"
#include "primitives/types.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/mocks/vm/states/state_manager_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/builtin/v2/account/account_actor.hpp"
#include "vm/version/version.hpp"

namespace fc::testutil::vm::actor::builtin {
  using ::fc::vm::actor::ActorVersion;
  using ::fc::vm::actor::CodeId;
  using ::fc::vm::actor::builtin::states::MockStateManager;
  using ::fc::vm::runtime::MockRuntime;
  using primitives::ChainEpoch;
  using primitives::address::Address;
  using storage::ipfs::InMemoryDatastore;
  using testing::_;
  using testing::Return;

  /**
   * Fixture class for actor testing
   */
  template <typename State>
  class ActorTestFixture : public testing::Test {
   public:
    void SetUp() override {
      EXPECT_CALL(runtime, getActorVersion())
          .WillRepeatedly(testing::Invoke([&]() { return actor_version; }));

      EXPECT_CALL(runtime, getIpfsDatastore())
          .WillRepeatedly(testing::Invoke([&]() { return ipld; }));

      EXPECT_CALL(runtime, tryResolveAddress(_))
          .WillRepeatedly(testing::Invoke([&](const auto &address) {
            auto found = resolve_addresses.find(address);
            if (found != resolve_addresses.end()) return found->second;
            return address;
          }));

      EXPECT_CALL(runtime, hashBlake2b(testing::_))
          .WillRepeatedly(testing::Invoke(
              [&](auto &data) { return crypto::blake2b::blake2b_256(data); }));

      EXPECT_CALL(runtime, getActorCodeID(_))
          .WillRepeatedly(testing::Invoke([&](auto &address) {
            auto found = code_ids.find(address);
            if (found != code_ids.end())
              return outcome::result<CodeId>(found->second);
            if (code_id_any.has_value())
              return outcome::result<CodeId>(code_id_any.value());
            return outcome::result<CodeId>(
                storage::ipfs::IpfsDatastoreError::kNotFound);
          }));

      EXPECT_CALL(runtime, getNetworkVersion())
          .WillRepeatedly(testing::Invoke([&]() {
            return fc::vm::version::getNetworkVersion(
                runtime.getCurrentEpoch());
          }));

      EXPECT_CALL(runtime, stateManager())
          .WillRepeatedly(testing::Return(state_manager));

      EXPECT_CALL(*state_manager, commitState(testing::_))
          .WillRepeatedly(testing::Invoke([&](const auto &s) {
            auto temp_state = std::static_pointer_cast<State>(s);
            EXPECT_OUTCOME_TRUE(cid, ipld->setCbor(*temp_state));
            EXPECT_OUTCOME_TRUE(new_state, ipld->getCbor<State>(cid));
            state = std::move(new_state);
            return outcome::success();
          }));
    }

    void callerIs(const Address &caller) {
      EXPECT_CALL(runtime, getImmediateCaller()).WillRepeatedly(Return(caller));
    }

    void currentEpochIs(const ChainEpoch &epoch) {
      current_epoch = epoch;
      EXPECT_CALL(runtime, getCurrentEpoch()).WillRepeatedly(Return(epoch));
    }

    /**
     * Expect V0 Account Actor method PubkeyAddress call
     * @param address - to get public key
     * @param bls_pubkey - to return
     */
    void expectAccountV0PubkeyAddressSend(const Address &address,
                                          const Blob<48> &bls_pubkey) {
      const Address bls_address = Address::makeBls(bls_pubkey);
      runtime.expectSendM<::fc::vm::actor::builtin::v0::account::PubkeyAddress>(
          address, {}, 0, {bls_address});
    }

    /**
     * Expect V2 Account Actor method PubkeyAddress call
     * @param address - to get public key
     * @param bls_pubkey - to return
     */
    void expectAccountV2PubkeyAddressSend(const Address &address,
                                          const Blob<48> &bls_pubkey) {
      const Address bls_address = Address::makeBls(bls_pubkey);
      runtime.expectSendM<::fc::vm::actor::builtin::v2::account::PubkeyAddress>(
          address, {}, 0, {bls_address});
    }

    void addressCodeIdIs(const Address &address, const CodeId &code_id) {
      code_ids[address] = code_id;
    }

    /**
     * Resolve any address code id as default one
     * @param code_id - default code id for all addresses
     */
    void anyCodeIdAddressIs(const CodeId &code_id) {
      code_id_any = code_id;
    }

    void resolveAddressAs(const Address &address, const Address &resolved) {
      resolve_addresses[address] = resolved;
    }

    MockRuntime runtime;
    std::shared_ptr<MockStateManager> state_manager{
        std::make_shared<MockStateManager>()};
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    State state;
    ChainEpoch current_epoch{};
    std::map<Address, CodeId> code_ids;
    boost::optional<CodeId> code_id_any;
    std::map<Address, Address> resolve_addresses;
    ActorVersion actor_version;
  };
}  // namespace fc::testutil::vm::actor::builtin
