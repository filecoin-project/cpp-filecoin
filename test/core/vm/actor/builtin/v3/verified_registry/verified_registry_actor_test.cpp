/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/verified_registry/verified_registry_actor.hpp"

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "vm/actor/actor.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace VerifiedRegistry = fc::vm::actor::builtin::v3::verified_registry;
using fc::primitives::address::Address;
using fc::storage::ipfs::InMemoryDatastore;
using fc::vm::VMAbortExitCode;
using fc::vm::VMExitCode;
using fc::vm::actor::kStorageMarketAddress;
using fc::vm::actor::kSystemActorAddress;
using fc::vm::runtime::MockRuntime;
using fc::vm::state::StateTreeImpl;
using testing::Return;
using VerifiedRegistry::DataCap;
using VerifiedRegistry::kMinVerifiedDealSize;
using VerifiedRegistry::StoragePower;

struct VerifiedRegistryActorTest : testing::Test {
  void SetUp() override {
    EXPECT_CALL(runtime, getIpfsDatastore())
        .Times(testing::AnyNumber())
        .WillRepeatedly(Return(ipld));

    EXPECT_CALL(runtime, resolveAddress(testing::_))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Invoke(
            [&](auto &address) { return state_tree.lookupId(address); }));

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
        .Times(testing::AtMost(1))
        .WillOnce(testing::Invoke([&](auto &cid) {
          EXPECT_OUTCOME_TRUE(new_state,
                              ipld->getCbor<VerifiedRegistry::State>(cid));
          state = std::move(new_state);
          return fc::outcome::success();
        }));

    ipld->load(state);
    setupState();
  }

  void setupState() {
    state.root_key = root_key;
  }

  MockRuntime runtime;

  std::shared_ptr<InMemoryDatastore> ipld{
      std::make_shared<InMemoryDatastore>()};

  Address caller{Address::makeFromId(101)};
  Address root_key{Address::makeFromId(102)};
  Address wrong_caller{Address::makeFromId(999)};

  Address verifier{Address::makeFromId(103)};
  Address verified_client{Address::makeFromId(104)};

  VerifiedRegistry::State state;

  StateTreeImpl state_tree{ipld};
};

/// VerifiedRegistryActor Construct error: caller is not system actor
TEST_F(VerifiedRegistryActorTest, ConstructCallerNotSystem) {
  caller = wrong_caller;

  EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrForbidden,
                       VerifiedRegistry::Construct::call(runtime, {}));
}

/// VerifiedRegistryActor Construct success
TEST_F(VerifiedRegistryActorTest, ConstructSuccess) {
  caller = kSystemActorAddress;

  EXPECT_OUTCOME_TRUE_1(VerifiedRegistry::Construct::call(runtime, root_key));
}

/// VerifiedRegistryActor AddVerifier error: allowance < kMinVerifiedDealSize
TEST_F(VerifiedRegistryActorTest, AddVerifierWrongAllowance) {
  DataCap allowance = 0;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::AddVerifier::call(runtime, {{}, allowance}));

  allowance = kMinVerifiedDealSize - 1;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::AddVerifier::call(runtime, {{}, allowance}));
}

/// VerifiedRegistryActor AddVerifier error: caller is not root key
TEST_F(VerifiedRegistryActorTest, AddVerifierCallerNotRootKey) {
  caller = wrong_caller;
  const DataCap allowance = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kSysErrForbidden,
      VerifiedRegistry::AddVerifier::call(runtime, {{}, allowance}));
}

/// VerifiedRegistryActor AddVerifier error: params address is a root key
TEST_F(VerifiedRegistryActorTest, AddVerifierAddressIsRootKey) {
  caller = root_key;
  const DataCap allowance = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::AddVerifier::call(runtime, {root_key, allowance}));
}

/// VerifiedRegistryActor AddVerifier error:
/// verifier already exists as verified client
TEST_F(VerifiedRegistryActorTest, AddVerifierWrongVerifier) {
  caller = root_key;
  const DataCap allowance = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_TRUE_1(state.verified_clients.set(verifier, 0));

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::AddVerifier::call(runtime, {verifier, allowance}));
}

/// VerifiedRegistryActor AddVerifier success
TEST_F(VerifiedRegistryActorTest, AddVerifierSuccess) {
  caller = root_key;
  const DataCap allowance = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_TRUE_1(
      VerifiedRegistry::AddVerifier::call(runtime, {verifier, allowance}));

  EXPECT_OUTCOME_TRUE(result, state.verifiers.get(verifier));
  EXPECT_EQ(result, allowance);
}

/// VerifiedRegistryActor RemoveVerifier error: caller is not root key
TEST_F(VerifiedRegistryActorTest, RemoveVerifierCallerNotRootKey) {
  caller = wrong_caller;

  EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrForbidden,
                       VerifiedRegistry::RemoveVerifier::call(runtime, {}));
}

/// VerifiedRegistryActor RemoveVerifier error: verifier doesn't exist
TEST_F(VerifiedRegistryActorTest, RemoveVerifierWrongVerifier) {
  caller = root_key;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalState),
      VerifiedRegistry::RemoveVerifier::call(runtime, verifier));
}

/// VerifiedRegistryActor RemoveVerifier success
TEST_F(VerifiedRegistryActorTest, RemoveVerifierSuccess) {
  caller = root_key;

  EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, 0));

  EXPECT_OUTCOME_TRUE_1(
      VerifiedRegistry::RemoveVerifier::call(runtime, verifier));

  EXPECT_OUTCOME_TRUE(result, state.verifiers.tryGet(verifier));
  EXPECT_EQ(result, boost::none);
}

/// VerifiedRegistryActor AddVerifiedClient error:
/// allowance < kMinVerifiedDealSize
TEST_F(VerifiedRegistryActorTest, AddVerifiedClientWrongAllowance) {
  DataCap allowance = 0;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::AddVerifiedClient::call(runtime, {{}, allowance}));

  allowance = kMinVerifiedDealSize - 1;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::AddVerifiedClient::call(runtime, {{}, allowance}));
}

/// VerifiedRegistryActor AddVerifiedClient error: caller is a root key
TEST_F(VerifiedRegistryActorTest, AddVerifiedClientClientIsRootKey) {
  const DataCap allowance = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_ERROR(ABORT_CAST(VMExitCode::kErrIllegalArgument),
                       VerifiedRegistry::AddVerifiedClient::call(
                           runtime, {root_key, allowance}));
}

/// VerifiedRegistryActor AddVerifiedClient error: caller is not a verifier
TEST_F(VerifiedRegistryActorTest, AddVerifiedClientCallerIsNotVerifier) {
  const DataCap allowance = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_ERROR(ABORT_CAST(VMExitCode::kErrNotFound),
                       VerifiedRegistry::AddVerifiedClient::call(
                           runtime, {verified_client, allowance}));
}

/// VerifiedRegistryActor AddVerifiedClient error: client is a verifier
TEST_F(VerifiedRegistryActorTest, AddVerifiedClientClientIsVerifier) {
  caller = verifier;
  const DataCap allowance = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, 0));
  EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verified_client, 0));

  EXPECT_OUTCOME_ERROR(ABORT_CAST(VMExitCode::kErrIllegalArgument),
                       VerifiedRegistry::AddVerifiedClient::call(
                           runtime, {verified_client, allowance}));
}

/// VerifiedRegistryActor AddVerifiedClient error:
/// verifier's allowance < params allowance
TEST_F(VerifiedRegistryActorTest, AddVerifiedClientWrongVerifierAllowance) {
  caller = verifier;
  const DataCap allowance = kMinVerifiedDealSize + 10;

  EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, 0));

  EXPECT_OUTCOME_ERROR(ABORT_CAST(VMExitCode::kErrIllegalArgument),
                       VerifiedRegistry::AddVerifiedClient::call(
                           runtime, {verified_client, allowance}));

  EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, allowance - 1));

  EXPECT_OUTCOME_ERROR(ABORT_CAST(VMExitCode::kErrIllegalArgument),
                       VerifiedRegistry::AddVerifiedClient::call(
                           runtime, {verified_client, allowance}));
}

/// VerifiedRegistryActor AddVerifiedClient error: client already exists
TEST_F(VerifiedRegistryActorTest, AddVerifiedClientClientAlreadyExists) {
  caller = verifier;
  const DataCap allowance = kMinVerifiedDealSize + 10;

  EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, allowance + 1));
  EXPECT_OUTCOME_TRUE_1(state.verified_clients.set(verified_client, 0));

  EXPECT_OUTCOME_ERROR(ABORT_CAST(VMExitCode::kErrIllegalArgument),
                       VerifiedRegistry::AddVerifiedClient::call(
                           runtime, {verified_client, allowance}));
}

/// VerifiedRegistryActor AddVerifiedClient success
TEST_F(VerifiedRegistryActorTest, AddVerifiedClientSuccess) {
  caller = verifier;
  const DataCap allowance = kMinVerifiedDealSize + 10;
  const DataCap delta = 25;

  EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, allowance + delta));

  EXPECT_OUTCOME_TRUE_1(VerifiedRegistry::AddVerifiedClient::call(
      runtime, {verified_client, allowance}));

  EXPECT_OUTCOME_TRUE(verifier_cap, state.verifiers.get(verifier));
  EXPECT_EQ(verifier_cap, delta);

  EXPECT_OUTCOME_TRUE(client_cap, state.verified_clients.get(verified_client));
  EXPECT_EQ(client_cap, allowance);
}

/// VerifiedRegistryActor UseBytes error: caller is not Storage Market
TEST_F(VerifiedRegistryActorTest, UseBytesWrongCaller) {
  caller = wrong_caller;

  EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrForbidden,
                       VerifiedRegistry::UseBytes::call(runtime, {}));
}

/// VerifiedRegistryActor UseBytes error: deal size < kMinVerifiedDealSize
TEST_F(VerifiedRegistryActorTest, UseBytesWrongDealSize) {
  caller = kStorageMarketAddress;

  StoragePower deal_size = 0;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::UseBytes::call(runtime, {{}, deal_size}));

  deal_size = kMinVerifiedDealSize - 1;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::UseBytes::call(runtime, {{}, deal_size}));
}

/// VerifiedRegistryActor UseBytes error: client doesn't exist
TEST_F(VerifiedRegistryActorTest, UseBytesClientDoesNotExist) {
  caller = kStorageMarketAddress;
  const StoragePower deal_size = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrNotFound),
      VerifiedRegistry::UseBytes::call(runtime, {verified_client, deal_size}));
}

/// VerifiedRegistryActor UseBytes error: client has negative allowance
TEST_F(VerifiedRegistryActorTest, UseBytesWrongClientAllowance) {
  caller = kStorageMarketAddress;
  const StoragePower deal_size = kMinVerifiedDealSize + 1;
  const DataCap wrong_allowance = -1;

  EXPECT_OUTCOME_TRUE_1(
      state.verified_clients.set(verified_client, wrong_allowance));

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalState),
      VerifiedRegistry::UseBytes::call(runtime, {verified_client, deal_size}));
}

/// VerifiedRegistryActor UseBytes error: deal size > client's allowance
TEST_F(VerifiedRegistryActorTest, UseBytesTooBigDealSize) {
  caller = kStorageMarketAddress;
  const DataCap allowance = kMinVerifiedDealSize + 1;
  const StoragePower deal_size = allowance + 1;

  EXPECT_OUTCOME_TRUE_1(state.verified_clients.set(verified_client, allowance));

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::UseBytes::call(runtime, {verified_client, deal_size}));
}

/// VerifiedRegistryActor UseBytes success:
/// new client's allowance < kMinVerifiedDealSize and they are removed
TEST_F(VerifiedRegistryActorTest, UseBytesSuccessClientRemoved) {
  caller = kStorageMarketAddress;
  const DataCap allowance = kMinVerifiedDealSize + 2;
  const StoragePower deal_size = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_TRUE_1(state.verified_clients.set(verified_client, allowance));

  EXPECT_OUTCOME_TRUE_1(
      VerifiedRegistry::UseBytes::call(runtime, {verified_client, deal_size}));

  EXPECT_OUTCOME_TRUE(client_cap,
                      state.verified_clients.tryGet(verified_client));
  EXPECT_EQ(client_cap, boost::none);
}

/// VerifiedRegistryActor UseBytes success: client's allowance is changed
TEST_F(VerifiedRegistryActorTest, UseBytesSuccessClientStays) {
  caller = kStorageMarketAddress;
  const DataCap allowance = 3 * kMinVerifiedDealSize;
  const StoragePower deal_size = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_TRUE_1(state.verified_clients.set(verified_client, allowance));

  EXPECT_OUTCOME_TRUE_1(
      VerifiedRegistry::UseBytes::call(runtime, {verified_client, deal_size}));

  EXPECT_OUTCOME_TRUE(client_cap, state.verified_clients.get(verified_client));
  EXPECT_EQ(client_cap, allowance - deal_size);
}

/// VerifiedRegistryActor RestoreBytes error: caller is not Storage Market
TEST_F(VerifiedRegistryActorTest, RestoreBytesWrongCaller) {
  caller = wrong_caller;

  EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrForbidden,
                       VerifiedRegistry::RestoreBytes::call(runtime, {}));
}

/// VerifiedRegistryActor RestoreBytes error: deal size < kMinVerifiedDealSize
TEST_F(VerifiedRegistryActorTest, RestoreBytesWrongDealSize) {
  caller = kStorageMarketAddress;

  StoragePower deal_size = 0;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::RestoreBytes::call(runtime, {{}, deal_size}));

  deal_size = kMinVerifiedDealSize - 1;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::RestoreBytes::call(runtime, {{}, deal_size}));
}

/// VerifiedRegistryActor RestoreBytes error: address is root key
TEST_F(VerifiedRegistryActorTest, RestoreBytesWrongAddress) {
  caller = kStorageMarketAddress;
  const StoragePower deal_size = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_ERROR(
      ABORT_CAST(VMExitCode::kErrIllegalArgument),
      VerifiedRegistry::RestoreBytes::call(runtime, {root_key, deal_size}));
}

/// VerifiedRegistryActor RestoreBytes error: address is a verifier
TEST_F(VerifiedRegistryActorTest, RestoreBytesAddressIsVerifier) {
  caller = kStorageMarketAddress;
  const StoragePower deal_size = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verified_client, 0));

  EXPECT_OUTCOME_ERROR(ABORT_CAST(VMExitCode::kErrIllegalArgument),
                       VerifiedRegistry::RestoreBytes::call(
                           runtime, {verified_client, deal_size}));
}

/// VerifiedRegistryActor RestoreBytes success: add new client
TEST_F(VerifiedRegistryActorTest, RestoreBytesSuccessNewClient) {
  caller = kStorageMarketAddress;
  const StoragePower deal_size = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_TRUE_1(VerifiedRegistry::RestoreBytes::call(
      runtime, {verified_client, deal_size}));

  EXPECT_OUTCOME_TRUE(client_cap, state.verified_clients.get(verified_client));
  EXPECT_EQ(client_cap, deal_size);
}

/// VerifiedRegistryActor RestoreBytes success: change existing client
TEST_F(VerifiedRegistryActorTest, RestoreBytesSuccessExistingClientChanged) {
  caller = kStorageMarketAddress;
  const DataCap allowance = kMinVerifiedDealSize + 1;
  const StoragePower deal_size = kMinVerifiedDealSize + 1;

  EXPECT_OUTCOME_TRUE_1(state.verified_clients.set(verified_client, allowance));

  EXPECT_OUTCOME_TRUE_1(VerifiedRegistry::RestoreBytes::call(
      runtime, {verified_client, deal_size}));

  EXPECT_OUTCOME_TRUE(client_cap, state.verified_clients.get(verified_client));
  EXPECT_EQ(client_cap, allowance + deal_size);
}
