/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/verified_registry/verified_registry_actor.hpp"
#include "vm/actor/builtin/states/verified_registry/v3/verified_registry_actor_state.hpp"
#include "vm/actor/builtin/types/verified_registry/policy.hpp"

#include <gtest/gtest.h>
#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"

namespace fc::vm::actor::builtin::v3::verified_registry {
  using primitives::StoragePower;
  using primitives::address::Address;
  using states::DataCap;
  using testutil::vm::actor::builtin::ActorTestFixture;
  using types::verified_registry::kMinVerifiedDealSize;
  using vm::VMExitCode;

  struct VerifiedRegistryActorTest
      : public ActorTestFixture<VerifiedRegistryActorState> {
    void SetUp() override {
      ActorTestFixture<VerifiedRegistryActorState>::SetUp();
      actor_version = ActorVersion::kVersion3;
      ipld->actor_version = actor_version;
      setupState();
    }

    void setupState() {
      cbor_blake::cbLoadT(ipld, state);
      state.root_key = root_key;
    }

    Address root_key{Address::makeFromId(102)};
    Address wrong_caller{Address::makeFromId(999)};

    Address verifier{Address::makeFromId(103)};
    Address verified_client{Address::makeFromId(104)};
  };

  /// VerifiedRegistryActor Construct error: caller is not system actor
  TEST_F(VerifiedRegistryActorTest, ConstructCallerNotSystem) {
    callerIs(wrong_caller);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         Construct::call(runtime, {}));
  }

  /// VerifiedRegistryActor Construct success
  TEST_F(VerifiedRegistryActorTest, ConstructSuccess) {
    callerIs(kSystemActorAddress);

    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, root_key));
  }

  /// VerifiedRegistryActor AddVerifier error: allowance < kMinVerifiedDealSize
  TEST_F(VerifiedRegistryActorTest, AddVerifierWrongAllowance) {
    DataCap allowance = 0;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         AddVerifier::call(runtime, {{}, allowance}));

    allowance = kMinVerifiedDealSize - 1;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         AddVerifier::call(runtime, {{}, allowance}));
  }

  /// VerifiedRegistryActor AddVerifier error: caller is not root key
  TEST_F(VerifiedRegistryActorTest, AddVerifierCallerNotRootKey) {
    callerIs(wrong_caller);
    const DataCap allowance = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         AddVerifier::call(runtime, {{}, allowance}));
  }

  /// VerifiedRegistryActor AddVerifier error: params address is a root key
  TEST_F(VerifiedRegistryActorTest, AddVerifierAddressIsRootKey) {
    callerIs(root_key);
    const DataCap allowance = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         AddVerifier::call(runtime, {root_key, allowance}));
  }

  /// VerifiedRegistryActor AddVerifier error:
  /// verifier already exists as verified client
  TEST_F(VerifiedRegistryActorTest, AddVerifierWrongVerifier) {
    callerIs(root_key);
    const DataCap allowance = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_TRUE_1(state.verified_clients.set(verifier, 0));

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         AddVerifier::call(runtime, {verifier, allowance}));
  }

  /// VerifiedRegistryActor AddVerifier success
  TEST_F(VerifiedRegistryActorTest, AddVerifierSuccess) {
    callerIs(root_key);
    const DataCap allowance = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_TRUE_1(AddVerifier::call(runtime, {verifier, allowance}));

    EXPECT_OUTCOME_TRUE(result, state.verifiers.get(verifier));
    EXPECT_EQ(result, allowance);
  }

  /// VerifiedRegistryActor RemoveVerifier error: caller is not root key
  TEST_F(VerifiedRegistryActorTest, RemoveVerifierCallerNotRootKey) {
    callerIs(wrong_caller);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         RemoveVerifier::call(runtime, {}));
  }

  /// VerifiedRegistryActor RemoveVerifier error: verifier doesn't exist
  TEST_F(VerifiedRegistryActorTest, RemoveVerifierWrongVerifier) {
    callerIs(root_key);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalState),
                         RemoveVerifier::call(runtime, verifier));
  }

  /// VerifiedRegistryActor RemoveVerifier success
  TEST_F(VerifiedRegistryActorTest, RemoveVerifierSuccess) {
    callerIs(root_key);

    EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, 0));

    EXPECT_OUTCOME_TRUE_1(RemoveVerifier::call(runtime, verifier));

    EXPECT_OUTCOME_TRUE(result, state.verifiers.tryGet(verifier));
    EXPECT_EQ(result, boost::none);
  }

  /// VerifiedRegistryActor AddVerifiedClient error:
  /// allowance < kMinVerifiedDealSize
  TEST_F(VerifiedRegistryActorTest, AddVerifiedClientWrongAllowance) {
    DataCap allowance = 0;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         AddVerifiedClient::call(runtime, {{}, allowance}));

    allowance = kMinVerifiedDealSize - 1;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         AddVerifiedClient::call(runtime, {{}, allowance}));
  }

  /// VerifiedRegistryActor AddVerifiedClient error: caller is a root key
  TEST_F(VerifiedRegistryActorTest, AddVerifiedClientClientIsRootKey) {
    const DataCap allowance = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        AddVerifiedClient::call(runtime, {root_key, allowance}));
  }

  /// VerifiedRegistryActor AddVerifiedClient error: caller is not a verifier
  TEST_F(VerifiedRegistryActorTest, AddVerifiedClientCallerIsNotVerifier) {
    callerIs(wrong_caller);
    const DataCap allowance = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrNotFound),
        AddVerifiedClient::call(runtime, {verified_client, allowance}));
  }

  /// VerifiedRegistryActor AddVerifiedClient error: client is a verifier
  TEST_F(VerifiedRegistryActorTest, AddVerifiedClientClientIsVerifier) {
    callerIs(verifier);
    const DataCap allowance = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, 0));
    EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verified_client, 0));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        AddVerifiedClient::call(runtime, {verified_client, allowance}));
  }

  /// VerifiedRegistryActor AddVerifiedClient error:
  /// verifier's allowance < params allowance
  TEST_F(VerifiedRegistryActorTest, AddVerifiedClientWrongVerifierAllowance) {
    callerIs(verifier);
    const DataCap allowance = kMinVerifiedDealSize + 10;

    EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, 0));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        AddVerifiedClient::call(runtime, {verified_client, allowance}));

    EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, allowance - 1));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        AddVerifiedClient::call(runtime, {verified_client, allowance}));
  }

  /// VerifiedRegistryActor AddVerifiedClient error: client already exists
  TEST_F(VerifiedRegistryActorTest, AddVerifiedClientClientAlreadyExists) {
    callerIs(verifier);
    const DataCap allowance = kMinVerifiedDealSize + 10;

    EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, allowance + 1));
    EXPECT_OUTCOME_TRUE_1(state.verified_clients.set(verified_client, 0));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        AddVerifiedClient::call(runtime, {verified_client, allowance}));
  }

  /// VerifiedRegistryActor AddVerifiedClient success
  TEST_F(VerifiedRegistryActorTest, AddVerifiedClientSuccess) {
    callerIs(verifier);
    const DataCap allowance = kMinVerifiedDealSize + 10;
    const DataCap delta = 25;

    EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verifier, allowance + delta));

    EXPECT_OUTCOME_TRUE_1(
        AddVerifiedClient::call(runtime, {verified_client, allowance}));

    EXPECT_OUTCOME_TRUE(verifier_cap, state.verifiers.get(verifier));
    EXPECT_EQ(verifier_cap, delta);

    EXPECT_OUTCOME_TRUE(client_cap,
                        state.verified_clients.get(verified_client));
    EXPECT_EQ(client_cap, allowance);
  }

  /// VerifiedRegistryActor UseBytes error: caller is not Storage Market
  TEST_F(VerifiedRegistryActorTest, UseBytesWrongCaller) {
    callerIs(wrong_caller);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         UseBytes::call(runtime, {}));
  }

  /// VerifiedRegistryActor UseBytes error: deal size < kMinVerifiedDealSize
  TEST_F(VerifiedRegistryActorTest, UseBytesWrongDealSize) {
    callerIs(kStorageMarketAddress);

    StoragePower deal_size = 0;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UseBytes::call(runtime, {{}, deal_size}));

    deal_size = kMinVerifiedDealSize - 1;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UseBytes::call(runtime, {{}, deal_size}));
  }

  /// VerifiedRegistryActor UseBytes error: client doesn't exist
  TEST_F(VerifiedRegistryActorTest, UseBytesClientDoesNotExist) {
    callerIs(kStorageMarketAddress);
    const StoragePower deal_size = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrNotFound),
                         UseBytes::call(runtime, {verified_client, deal_size}));
  }

  /// VerifiedRegistryActor UseBytes error: client has negative allowance
  TEST_F(VerifiedRegistryActorTest, UseBytesWrongClientAllowance) {
    callerIs(kStorageMarketAddress);
    const StoragePower deal_size = kMinVerifiedDealSize + 1;
    const DataCap wrong_allowance = -1;

    EXPECT_OUTCOME_TRUE_1(
        state.verified_clients.set(verified_client, wrong_allowance));

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalState),
                         UseBytes::call(runtime, {verified_client, deal_size}));
  }

  /// VerifiedRegistryActor UseBytes error: deal size > client's allowance
  TEST_F(VerifiedRegistryActorTest, UseBytesTooBigDealSize) {
    callerIs(kStorageMarketAddress);
    const DataCap allowance = kMinVerifiedDealSize + 1;
    const StoragePower deal_size = allowance + 1;

    EXPECT_OUTCOME_TRUE_1(
        state.verified_clients.set(verified_client, allowance));

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UseBytes::call(runtime, {verified_client, deal_size}));
  }

  /// VerifiedRegistryActor UseBytes success:
  /// new client's allowance < kMinVerifiedDealSize and they are removed
  TEST_F(VerifiedRegistryActorTest, UseBytesSuccessClientRemoved) {
    callerIs(kStorageMarketAddress);
    const DataCap allowance = kMinVerifiedDealSize + 2;
    const StoragePower deal_size = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_TRUE_1(
        state.verified_clients.set(verified_client, allowance));

    EXPECT_OUTCOME_TRUE_1(
        UseBytes::call(runtime, {verified_client, deal_size}));

    EXPECT_OUTCOME_TRUE(client_cap,
                        state.verified_clients.tryGet(verified_client));
    EXPECT_EQ(client_cap, boost::none);
  }

  /// VerifiedRegistryActor UseBytes success: client's allowance is changed
  TEST_F(VerifiedRegistryActorTest, UseBytesSuccessClientStays) {
    callerIs(kStorageMarketAddress);
    const DataCap allowance = 3 * kMinVerifiedDealSize;
    const StoragePower deal_size = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_TRUE_1(
        state.verified_clients.set(verified_client, allowance));

    EXPECT_OUTCOME_TRUE_1(
        UseBytes::call(runtime, {verified_client, deal_size}));

    EXPECT_OUTCOME_TRUE(client_cap,
                        state.verified_clients.get(verified_client));
    EXPECT_EQ(client_cap, allowance - deal_size);
  }

  /// VerifiedRegistryActor RestoreBytes error: caller is not Storage Market
  TEST_F(VerifiedRegistryActorTest, RestoreBytesWrongCaller) {
    callerIs(wrong_caller);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         RestoreBytes::call(runtime, {}));
  }

  /// VerifiedRegistryActor RestoreBytes error: deal size < kMinVerifiedDealSize
  TEST_F(VerifiedRegistryActorTest, RestoreBytesWrongDealSize) {
    callerIs(kStorageMarketAddress);

    StoragePower deal_size = 0;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         RestoreBytes::call(runtime, {{}, deal_size}));

    deal_size = kMinVerifiedDealSize - 1;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         RestoreBytes::call(runtime, {{}, deal_size}));
  }

  /// VerifiedRegistryActor RestoreBytes error: address is root key
  TEST_F(VerifiedRegistryActorTest, RestoreBytesWrongAddress) {
    callerIs(kStorageMarketAddress);
    const StoragePower deal_size = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         RestoreBytes::call(runtime, {root_key, deal_size}));
  }

  /// VerifiedRegistryActor RestoreBytes error: address is a verifier
  TEST_F(VerifiedRegistryActorTest, RestoreBytesAddressIsVerifier) {
    callerIs(kStorageMarketAddress);
    const StoragePower deal_size = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_TRUE_1(state.verifiers.set(verified_client, 0));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        RestoreBytes::call(runtime, {verified_client, deal_size}));
  }

  /// VerifiedRegistryActor RestoreBytes success: add new client
  TEST_F(VerifiedRegistryActorTest, RestoreBytesSuccessNewClient) {
    callerIs(kStorageMarketAddress);
    const StoragePower deal_size = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_TRUE_1(
        RestoreBytes::call(runtime, {verified_client, deal_size}));

    EXPECT_OUTCOME_TRUE(client_cap,
                        state.verified_clients.get(verified_client));
    EXPECT_EQ(client_cap, deal_size);
  }

  /// VerifiedRegistryActor RestoreBytes success: change existing client
  TEST_F(VerifiedRegistryActorTest, RestoreBytesSuccessExistingClientChanged) {
    callerIs(kStorageMarketAddress);
    const DataCap allowance = kMinVerifiedDealSize + 1;
    const StoragePower deal_size = kMinVerifiedDealSize + 1;

    EXPECT_OUTCOME_TRUE_1(
        state.verified_clients.set(verified_client, allowance));

    EXPECT_OUTCOME_TRUE_1(
        RestoreBytes::call(runtime, {verified_client, deal_size}));

    EXPECT_OUTCOME_TRUE(client_cap,
                        state.verified_clients.get(verified_client));
    EXPECT_EQ(client_cap, allowance + deal_size);
  }

}  // namespace fc::vm::actor::builtin::v3::verified_registry
