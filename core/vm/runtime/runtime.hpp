/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_HPP

#include "crypto/randomness/randomness_provider.hpp"

namespace fc::vm::runtime {

  using crypto::randomness::RandomnessProvider;
  using crypto::randomness::Randomness;

  class Runtime {
   public:
    // getCurrentEpoch()

    // Randomness returns a (pseudo)random string for the given epoch and tag.

    Randomness getRandomness();
    // getRandomnessWithAuxSeed() -- ??

    // The address of the immediate calling actor.

    // Not necessarily the actor in the From field of the initial on-chain
    // Message. Always an ID-address.

    // ImmediateCaller() addr.Address
    // ValidateImmediateCallerIs(caller addr.Address)
    // ValidateImmediateCallerInSet(callers [addr.Address])
    // ValidateImmediateCallerAcceptAnyOfType(type_ actor.BuiltinActorID)
    // ValidateImmediateCallerAcceptAnyOfTypes(types [actor.BuiltinActorID])
    // ValidateImmediateCallerAcceptAny()
    // ValidateImmediateCallerMatches(CallerPattern)

    // The address of the actor receiving the message. Always an ID-address.
    //    CurrReceiver()         addr.Address

    // The actor who mined the block in which the initial on-chain message
    // appears. Always an ID-address.
    //    ToplevelBlockWinner()  addr.Address
    //
    //    AcquireState()         ActorStateHandle
    //
    //    SuccessReturn()        InvocOutput
    //    ValueReturn(Bytes)     InvocOutput

    // Throw an error indicating a failure condition has occurred, from which
    // the given actor code is unable to recover.
    //    Abort(errExitCode exitcode.ExitCode, msg string)

    //    // Calls Abort with InvalidArguments_User.
    //    AbortArgMsg(msg string)
    //    AbortArg()
    //
    //    // Calls Abort with InconsistentState_User.
    //    AbortStateMsg(msg string)
    //    AbortState()
    //
    //    // Calls Abort with InsufficientFunds_User.
    //    AbortFundsMsg(msg string)
    //    AbortFunds()
    //
    //    // Calls Abort with RuntimeAPIError.
    //    // For internal use only (not in actor code).
    //    AbortAPI(msg string)

    // Check that the given condition is true (and call Abort if not).
    //    Assert(bool)

    //    CurrentBalance()  actor.TokenAmount
    //    ValueReceived()   actor.TokenAmount

    // Look up the current values of several system-wide economic indices.
    //    CurrIndices()     indices.Indices

    // Look up the code ID of a given actor address.
    //    GetActorCodeID(addr addr.Address) (ret actor.CodeID, ok bool)

    // Run a (pure function) computation, consuming the gas cost associated with
    // that function. This mechanism is intended to capture the notion of an ABI
    // between the VM and native functions, and should be used for any function
    // whose computation is expensive.
    //    Compute(ComputeFunctionID, args [util.Any]) util.Any

    // Sends a message to another actor.
    // If the invoked method does not return successfully, this caller will be
    // aborted too.
    //    SendPropagatingErrors(input InvocInput) InvocOutput
    //    Send(
    //        toAddr     addr.Address
    //        methodNum  actor.MethodNum
    //        params     actor.MethodParams
    //        value      actor.TokenAmount
    //    ) InvocOutput
    //    SendQuery(
    //        toAddr     addr.Address
    //        methodNum  actor.MethodNum
    //        params     [util.Serialization]
    //    ) util.Serialization
    //    SendFunds(toAddr addr.Address, value actor.TokenAmount)

    // Sends a message to another actor, trapping an unsuccessful execution.
    // This may only be invoked by the singleton Cron actor.
    //    SendCatchingErrors(input InvocInput) (output InvocOutput, exitCode
    //    exitcode.ExitCode)

    // Computes an address for a new actor. The returned address is intended to
    // uniquely refer to the actor even in the event of a chain re-org (whereas
    // an ID-address might refer to a different actor after messages are
    // re-ordered). Always an ActorExec address.
    //    NewActorAddress() addr.Address

    // Creates an actor in the state tree, with empty state. May only be called
    // by InitActor.
    //    CreateActor(
    // The new actor's code identifier.
    //        codeId   actor.CodeID
    // Address under which the new actor's state will be stored. Must be an
    // ID-address.
    //        address  addr.Address
    //    )

    // Deletes an actor in the state tree. May only be called by the actor
    // itself, or by StoragePowerActor in the case of StorageMinerActors.
    //    DeleteActor(address addr.Address)

    // Retrieves and deserializes an object from the store into o. Returns
    // whether successful.
    //    IpldGet(c ipld.CID, o ipld.Object) bool
    // Serializes and stores an object, returning its CID.
    //    IpldPut(x ipld.Object) ipld.CID
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_HPP
