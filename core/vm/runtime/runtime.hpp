/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_HPP

#include <libp2p/multi/content_identifier.hpp>
#include <tuple>

#include "common/outcome.hpp"
#include "crypto/randomness/randomness_provider.hpp"
#include "primitives/address.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::vm::runtime {

  using actor::CodeId;
  using actor::MethodNumber;
  using actor::MethodParams;
  using actor::TokenAmount;
  using crypto::randomness::Randomness;
  using crypto::randomness::RandomnessProvider;
  using exit_code::ExitCode;
  using libp2p::multi::ContentIdentifier;
  using primitives::Address;
  using IpldObject = storage::ipfs::IpfsDatastore::Value;
  using Serialization = common::Buffer;

  class Runtime {
   public:
    virtual ChainEpoch getCurrentEpoch() const = 0;

    /**
     * @brief Returns a (pseudo)random string for the given epoch and tag.
     */
    virtual Randomness getRandomness(DomainSeparationTag tag,
                                     ChainEpoch epoch) const = 0;
    virtual Randomness getRandomnessWithAuxSeed(
        DomainSeparationTag tag,
        ChainEpoch epoch,
        Serialization auxSeed) const = 0;

    /**
     * The address of the immediate calling actor. Not necessarily the actor in
     * the From field of the initial on-chain Message. Always an ID-address.
     */
    virtual Address getImmediateCaller() const = 0;

    /** The address of the actor receiving the message. Always an ID-address. */
    virtual Address getCurrentReceiver() const = 0;

    /**
     * The actor who mined the block in which the initial on-chain message
     * appears. Always an ID-address.
     */
    virtual Address getTopLevelBlockWinner() const = 0;

    virtual ActorStateHandle acquireState() const = 0;

    virtual InvocationOutput returnSuccess() = 0;
    virtual InvocationOutput returnValue(Buffer bytes) = 0;

    /**
     * Throw an error indicating a failure condition has occurred, from which
     * the given actor code is unable to recover.
     */
    virtual void abort(const ExitCode &errExitCode, const std::string &msg) = 0;

    /** Calls Abort with kInvalidArgumentsUser. */
    virtual void abortArg(const std::string &message) = 0;
    virtual void abortArg() = 0;

    /** Calls Abort with kInconsistentStateUser. */
    virtual void abortState(const std::string &message) = 0;
    virtual void abortState() = 0;

    /** Calls Abort with InsufficientFunds_User. */
    virtual void abortFunds(const std::string &message) = 0;
    virtual void abortFunds() = 0;

    /**
     * Calls Abort with kRuntimeAPIError. For internal use only (not in actor
     * code).
     */
    virtual void abortAPI(const std::string &message) = 0;

    /** Check that the given condition is true (and call Abort if not). */
    virtual void assert(bool condition) = 0;

    virtual TokenAmount getCurrentBalance() const = 0;
    virtual TokenAmount getValueReceived() const = 0;

    /** Look up the current values of several system-wide economic indices. */
    // TODO(a.chernyshov) Indices type
    // CurrIndices()     indices.Indices

    /** Look up the code ID of a given actor address. */
    virtual outcome::result<CodeId> getActorCodeID(const Address &address) = 0;

    /**
     * Run a (pure function) computation, consuming the gas cost associated with
     * that function. This mechanism is intended to capture the notion of an ABI
     * between the VM and native functions, and should be used for any function
     * whose computation is expensive
     * @param function
     * @return
     */
    // TODO(a.chernyshov) util.Any type
    // Compute(ComputeFunctionID, args [util.Any]) util.Any

    /**
     * Sends a message to another actor. If the invoked method does not return
     * successfully, this caller will be aborted too.
     */
    virtual InvocationOutput sendPropagatingErrors(InvocationInput input) = 0;
    virtual InvocationOutput send(Address to_address,
                                  MethodNumber method_number,
                                  MethodParams params,
                                  TokenAmount value) = 0;
    virtual Serialization sendQuery(Address to_addr,
                                    MethodNumber method_nuberm,
                                    gsl::span<Serialization> params) = 0;
    virtual sendFunds(const Address &to_address, const TokenAmount &value) = 0;

    /**
     * Sends a message to another actor, trapping an unsuccessful execution.
     * This may only be invoked by the singleton Cron actor.
     */
    virtual std::tuple<InvocationOutput, ExitCode> sendCatchingErrors(
        InvocationInput input) = 0;

    // Computes an address for a new actor. The returned address is intended to
    // uniquely refer to the actor even in the event of a chain re-org (whereas
    // an ID-address might refer to a different actor after messages are
    // re-ordered). Always an ActorExec address.
    virtual Address createNewActorAddress() = 0;

    /**
     * @brief Creates an actor in the state tree, with empty state. May only be
     * called by InitActor
     * @param code_id - the new actor's code identifier
     * @param address - Address under which the new actor's state will be
     * stored. Must be an ID-address
     */
    virtual outcome::result<void> createActor(CodeId code_id,
                                              const Address &address) = 0;

    // Deletes an actor in the state tree. May only be called by the actor
    // itself, or by StoragePowerActor in the case of StorageMinerActors.
    virtual void deleteActor(const Address &address) = 0;

    // Retrieves and deserializes an object from the store into o. Returns
    // whether successful.
    outcome::result<IpldObject> getIpld(ContentIdentifier cid);

    // Serializes and stores an object, returning its CID.
    ContentIdentifier putIpld(IpldObject object);
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_HPP
