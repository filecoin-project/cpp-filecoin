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
#include "primitives/block/block.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor_encoding.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/indices/indices.hpp"
#include "vm/message/message.hpp"
#include "vm/runtime/actor_state_handle.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::vm::runtime {

  using actor::Actor;
  using actor::CodeId;
  using actor::kSendMethodNumber;
  using actor::MethodNumber;
  using actor::MethodParams;
  using common::Buffer;
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;
  using exit_code::ExitCode;
  using indices::Indices;
  using message::UnsignedMessage;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::block::BlockHeader;
  using primitives::sector::PoStVerifyInfo;
  using primitives::sector::SealVerifyInfo;
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
     * @param address - Address under which the new actor's state will be
     * stored. Must be an ID-address
     * @param actor - Actor state
     */
    virtual outcome::result<void> createActor(const Address &address,
                                              const Actor &actor) = 0;

    /**
     * @brief Deletes an actor in the state tree
     *
     * May only be called by the actor itself
     */
    virtual outcome::result<void> deleteActor() = 0;

    /**
     * @brief Returns IPFS datastore
     */
    virtual std::shared_ptr<IpfsDatastore> getIpfsDatastore() = 0;

    /**
     * Get Message for actor invocation
     * @return message invoking current execution
     */
    virtual std::reference_wrapper<const UnsignedMessage> getMessage() = 0;

    /// Try to charge gas or throw if there is not enoght gas
    virtual outcome::result<void> chargeGas(const BigInt &amount) = 0;

    /// Get current actor state root CID
    virtual ActorSubstateCID getCurrentActorState() = 0;

    /// Update actor state CID
    virtual outcome::result<void> commit(const ActorSubstateCID &new_state) = 0;

    /// Resolve address to id-address
    virtual outcome::result<Address> resolveAddress(const Address &address) = 0;

    /// Verify PoSt
    virtual outcome::result<bool> verifyPoSt(const PoStVerifyInfo &info) = 0;

    /// Verify seal
    virtual outcome::result<bool> verifySeal(const SealVerifyInfo &info) = 0;

    /// Verify consensus fault
    virtual outcome::result<bool> verifyConsensusFault(
        const BlockHeader &block_header_1,
        const BlockHeader &block_header_2) = 0;

    /// Send typed method with typed params and result
    template <typename M>
    outcome::result<typename M::Result> sendM(const Address &address,
                                              const typename M::Params &params,
                                              TokenAmount value) {
      OUTCOME_TRY(params2, actor::encodeActorParams(params));
      OUTCOME_TRY(result, send(address, M::Number, params2, value));
      return actor::decodeActorReturn<typename M::Result>(result);
    }

    /// Send funds
    inline auto sendFunds(const Address &to, BigInt value) {
      return send(to, kSendMethodNumber, {}, value);
    }

    /// Send with typed result R
    template <typename R>
    outcome::result<R> sendR(Address to_address,
                             MethodNumber method_number,
                             const MethodParams &params,
                             BigInt value) {
      OUTCOME_TRY(result, send(to_address, method_number, params, value));
      return codec::cbor::decode<R>(result.return_value);
    }

    /// Send with typed params P and result R
    template <typename R, typename P>
    outcome::result<R> sendPR(Address to_address,
                              MethodNumber method_number,
                              const P &params,
                              BigInt value) {
      OUTCOME_TRY(params2, actor::encodeActorParams(params));
      return sendR<R>(to_address, method_number, MethodParams{params2}, value);
    }

    /// Send with typed params P
    template <typename P>
    outcome::result<InvocationOutput> sendP(Address to_address,
                                            MethodNumber method_number,
                                            const P &params,
                                            BigInt value) {
      OUTCOME_TRY(params2, actor::encodeActorParams(params));
      return send(to_address, method_number, MethodParams{params2}, value);
    }

    /// Get decoded current actor state
    template <typename T>
    outcome::result<T> getCurrentActorStateCbor() {
      return getIpfsDatastore()->getCbor<T>(getCurrentActorState());
    }

    /**
     * Commit actor state
     * @tparam T - POD state type
     * @param state - actor state structure
     * @return error in case of failure
     */
    template <typename T>
    outcome::result<void> commitState(const T &state) {
      OUTCOME_TRY(state_cid, getIpfsDatastore()->setCbor(state));
      OUTCOME_TRY(commit(ActorSubstateCID{state_cid}));
      return outcome::success();
    }
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_RUNTIME_HPP
