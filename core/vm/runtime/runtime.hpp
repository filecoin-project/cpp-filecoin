/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <tuple>

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "common/outcome.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "crypto/randomness/randomness_types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/block/block.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/actor_encoding.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/message/message.hpp"
#include "vm/runtime/runtime_types.hpp"
#include "vm/version.hpp"

/**
 * Aborts execution if res has error and aborts with default_error if res has
 * error and the error is not fatal or abort
 */
#define REQUIRE_NO_ERROR(expr, err_code) \
  OUTCOME_TRY(Runtime::requireNoError((expr), (err_code)));

/**
 * Return VMExitCode as VMAbortExitCode for special handling
 */
#define ABORT_CAST(err_code) static_cast<VMAbortExitCode>(err_code)

namespace fc::vm::runtime {

  using actor::Actor;
  using actor::CodeId;
  using actor::isStorageMinerActor;
  using actor::kSendMethodNumber;
  using actor::MethodNumber;
  using actor::MethodParams;
  using common::Buffer;
  using crypto::blake2b::Blake2b256Hash;
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;
  using crypto::signature::Signature;
  using message::UnsignedMessage;
  using primitives::ChainEpoch;
  using primitives::GasAmount;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::block::BlockHeader;
  using primitives::piece::PieceInfo;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::SealVerifyInfo;
  using primitives::sector::WindowPoStVerifyInfo;
  using storage::ipfs::IpfsDatastore;
  using version::NetworkVersion;

  struct Execution;

  /**
   * @class Runtime is the VM's internal runtime object exposed to actors
   */
  class Runtime {
   public:
    virtual ~Runtime() = default;

    virtual std::shared_ptr<Execution> execution() const = 0;

    virtual NetworkVersion getNetworkVersion() const = 0;

    /**
     * Returns current chain epoch which is equal to block chain height.
     * @return current chain epoch
     */
    virtual ChainEpoch getCurrentEpoch() const = 0;

    /**
     * @brief Returns a (pseudo)random string for the given epoch and tag.
     */
    virtual outcome::result<Randomness> getRandomnessFromTickets(
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const = 0;

    virtual outcome::result<Randomness> getRandomnessFromBeacon(
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const = 0;

    /**
     * The address of the immediate calling actor. Not necessarily the actor in
     * the From field of the initial on-chain Message. Always an ID-address.
     */
    virtual Address getImmediateCaller() const = 0;

    /** The address of the actor receiving the message. Always an ID-address. */
    virtual Address getCurrentReceiver() const = 0;

    virtual outcome::result<BigInt> getBalance(
        const Address &address) const = 0;

    virtual BigInt getValueReceived() const = 0;

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

    /** Computes an address for a new actor.
     *
     * The returned address is intended to uniquely refer to the actor even in
     * the event of a chain re-org (whereas an ID-address might refer to a
     * different actor after messages are re-ordered). Always an ActorExec
     * address.
     *
     * @return new unique actor address
     */
    virtual outcome::result<Address> createNewActorAddress() = 0;

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
     * @param address - Address of actor that receives remaining balance
     *
     * May only be called by the actor itself
     */
    virtual outcome::result<void> deleteActor(const Address &address) = 0;

    /**
     * @brief Transfer debits money from one account and credits it to another
     *
     * @param debitFrom - money sender
     * @param creditTo - money receiver
     * @param amount - amount of money that is transfered
     */
    virtual outcome::result<void> transfer(const Address &debitFrom,
                                           const Address &creditTo,
                                           const TokenAmount &amount) = 0;

    /**
     * Returns the total token supply in circulation at the beginning of the
     * current epoch.
     * The circulating supply is the sum of:
     * - rewards emitted by the reward actor,
     * - funds vested from lock-ups in the genesis state,
     * less the sum of:
     * - funds burnt,
     * - pledge collateral locked in storage miner actors (recorded in the
     * storage power actor)
     * - deal collateral locked by the storage market actor
     */
    virtual fc::outcome::result<TokenAmount> getTotalFilCirculationSupply()
        const = 0;

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
    virtual outcome::result<void> chargeGas(GasAmount amount) = 0;

    /// Get current actor state root CID
    virtual outcome::result<CID> getCurrentActorState() = 0;

    /// Update actor state CID
    virtual outcome::result<void> commit(const CID &new_state) = 0;

    /// Resolve address to id-address
    virtual outcome::result<Address> resolveAddress(
        const Address &address) const = 0;

    /// Verify signature
    virtual outcome::result<bool> verifySignature(
        const Signature &signature,
        const Address &address,
        gsl::span<const uint8_t> data) = 0;

    virtual outcome::result<bool> verifySignatureBytes(
        const Buffer &signature_bytes,
        const Address &address,
        gsl::span<const uint8_t> data) = 0;

    /// Verify PoSt
    virtual outcome::result<bool> verifyPoSt(
        const WindowPoStVerifyInfo &info) = 0;

    virtual outcome::result<std::map<Address, std::vector<bool>>>
    verifyBatchSeals(const adt::Map<adt::Array<SealVerifyInfo>,
                                    adt::AddressKeyer> &seals) = 0;

    /// Compute unsealed sector cid
    virtual outcome::result<CID> computeUnsealedSectorCid(
        RegisteredSealProof type, const std::vector<PieceInfo> &pieces) = 0;

    /// Verify consensus fault
    virtual outcome::result<ConsensusFault> verifyConsensusFault(
        const Buffer &block1, const Buffer &block2, const Buffer &extra) = 0;

    /// Return a hash of data
    virtual outcome::result<Blake2b256Hash> hashBlake2b(
        gsl::span<const uint8_t> data) = 0;

    /**
     * Aborts execution if res has error
     * @tparam T - result type
     * @param res - result to check
     * @param default_error - default VMExitCode to abort with.
     * @return If res has no error, success() returned. Otherwise if res.error()
     * is VMAbortExitCode or VMFatal, the res.error() returned, else
     * default_error returned
     */
    template <typename T>
    static outcome::result<void> requireNoError(
        const outcome::result<T> &res, const VMExitCode &default_error) {
      if (res.has_error()) {
        if (isFatal(res.error()) || isAbortExitCode(res.error())) {
          return res.error();
        }
        if (isVMExitCode(res.error())) {
          return ABORT_CAST(VMExitCode{res.error().value()});
        }
        return ABORT_CAST(default_error);
      }
      return outcome::success();
    }

    /**
     * Abort execution with VMExitCode
     * @param error_code - error code that should be passed to the caller
     * @return error_code as VMAbortExitCode
     */
    outcome::result<void> abort(const VMExitCode &error_code) const {
      return VMAbortExitCode{error_code};
    }

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
      return codec::cbor::decode<R>(result);
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
      OUTCOME_TRY(head, getCurrentActorState());
      return getIpfsDatastore()->getCbor<T>(head);
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
      OUTCOME_TRY(commit(state_cid));
      return outcome::success();
    }

    inline operator std::shared_ptr<IpfsDatastore>() {
      return getIpfsDatastore();
    }

    inline auto getCurrentBalance() {
      return getBalance(getCurrentReceiver());
    }

    inline outcome::result<void> validateArgument(bool assertion) const {
      if (!assertion) {
        return abort(VMExitCode::kErrIllegalArgument);
      }
      return outcome::success();
    }

    inline outcome::result<void> validateImmediateCallerIs(
        const Address &address) {
      if (getImmediateCaller() == address) {
        return outcome::success();
      }
      return VMExitCode::kSysErrForbidden;
    }

    inline outcome::result<void> validateImmediateCallerIs(
        std::initializer_list<Address> addresses) {
      for (const auto &address : addresses) {
        if (getImmediateCaller() == address) {
          return outcome::success();
        }
      }
      return VMExitCode::kSysErrForbidden;
    }

    inline outcome::result<void> validateImmediateCallerType(
        const CID &expected_code) {
      OUTCOME_TRY(actual_code, getActorCodeID(getImmediateCaller()));
      if (actual_code == expected_code) {
        return outcome::success();
      }
      return VMExitCode::kSysErrForbidden;
    }

    inline outcome::result<void> validateImmediateCallerIsSignable() {
      OUTCOME_TRY(code, getActorCodeID(getImmediateCaller()));
      if (actor::isSignableActor(code)) {
        return outcome::success();
      }
      return VMExitCode::kSysErrForbidden;
    }

    inline outcome::result<void> validateImmediateCallerIsMiner() {
      OUTCOME_TRY(actual_code, getActorCodeID(getImmediateCaller()));
      if (isStorageMinerActor(actual_code)) {
        return outcome::success();
      }
      return VMExitCode::kSysErrForbidden;
    }

    inline outcome::result<void> validateImmediateCallerIsCurrentReceiver() {
      if (getImmediateCaller() == getCurrentReceiver()) {
        return outcome::success();
      }
      return VMExitCode::kSysErrForbidden;
    }
  };

}  // namespace fc::vm::runtime
