/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_HPP

#include <tuple>

#include "common/outcome.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/indices/indices.hpp"
#include "vm/message/message.hpp"
#include "vm/runtime/actor_state_handle.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::vm::runtime {

  using actor::Actor;
  using actor::CodeId;
  using actor::MethodNumber;
  using actor::MethodParams;
  using common::Buffer;
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;
  using exit_code::ExitCode;
  using indices::Indices;
  using message::UnsignedMessage;
  using primitives::ChainEpoch;
  using primitives::address::Address;
  using storage::ipfs::IpfsDatastore;
  using Serialization = Buffer;

  /**
   * @class Runtime is the VM's internal runtime object exposed to actors
   */
  class Runtime {
   public:
    virtual ~Runtime() = default;

    /**
     * Returns current chain epoch which is equal to block chain height.
     * @return current chain epoch
     */
    virtual ChainEpoch getCurrentEpoch() const = 0;

    /**
     * @brief Returns a (pseudo)random string for the given epoch and tag.
     */
    virtual Randomness getRandomness(DomainSeparationTag tag,
                                     ChainEpoch epoch) const = 0;
    virtual Randomness getRandomness(DomainSeparationTag tag,
                                     ChainEpoch epoch,
                                     Serialization seed) const = 0;

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

    virtual std::shared_ptr<ActorStateHandle> acquireState() const = 0;

    virtual outcome::result<BigInt> getBalance(
        const Address &address) const = 0;

    virtual BigInt getValueReceived() const = 0;

    /** Look up the current values of several system-wide economic indices. */
    virtual std::shared_ptr<Indices> getCurrentIndices() const = 0;

    /** Look up the code ID of a given actor address. */
    virtual outcome::result<CodeId> getActorCodeID(
        const Address &address) const = 0;

    /**
     * Send allows the current execution context to invoke methods on other
     * actors in the system
     * @param to_address - message recipient
     * @param method_number - method number to invoke
     * @param params - method params
     * @param value - amount transferred
     * @return
     */
    virtual outcome::result<InvocationOutput> send(Address to_address,
                                                   MethodNumber method_number,
                                                   MethodParams params,
                                                   BigInt value) = 0;

    /**
     * @brief Creates an actor in the state tree, with empty state. May only be
     * called by InitActor
     * @brief Saves an actor in the state tree. May only be called by InitActor
     * @param address - Address under which the new actor's state will be
     * stored. Must be an ID-address
     * @param actor - Actor state
     */
    virtual outcome::result<void> createActor(const Address &address,
                                              const Actor &actor) = 0;

    /**
     * @brief Deletes an actor in the state tree
     *
     * May only be called by the actor itself, or by StoragePowerActor in the
     * case of StorageMinerActors
     * @param address - address of actor to delete
     */
    virtual outcome::result<void> deleteActor(const Address &address) = 0;

    /**
     * @brief Returns IPFS datastore
     */
    virtual std::shared_ptr<IpfsDatastore> getIpfsDatastore() = 0;

    /**
     * Get Message for actor invocation
     * @return message invoking current execution
     */
    virtual std::reference_wrapper<const UnsignedMessage> getMessage() = 0;

    virtual outcome::result<void> chargeGas(const BigInt &amount) = 0;

    virtual ActorSubstateCID getHead() = 0;

    virtual outcome::result<void> commit(const ActorSubstateCID &new_head) = 0;
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_HPP
