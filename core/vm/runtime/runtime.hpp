/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <tuple>

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "common/error_text.hpp"
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
#include "vm/version/version.hpp"

#define VM_ASSERT(condition) OUTCOME_TRY(runtime.vm_assert(condition))

namespace fc::vm::runtime {

  using actor::Actor;
  using actor::ActorVersion;
  using actor::CodeId;
  using actor::kSendMethodNumber;
  using actor::MethodNumber;
  using actor::MethodParams;
  using crypto::blake2b::Blake2b256Hash;
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;
  using crypto::signature::Signature;
  using message::UnsignedMessage;
  using primitives::ChainEpoch;
  using primitives::GasAmount;
  using primitives::SectorNumber;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::block::BlockHeader;
  using primitives::piece::PieceInfo;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::SealVerifyInfo;
  using primitives::sector::WindowPoStVerifyInfo;
  using storage::ipfs::IpfsDatastore;
  using version::NetworkVersion;
  using BatchSealsIn =
      std::vector<std::pair<Address, std::vector<SealVerifyInfo>>>;
  using BatchSealsOut =
      std::vector<std::pair<Address, std::vector<SectorNumber>>>;

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
     * Returns current version of actor
     * @return crrent version of actor
     */
    virtual ActorVersion getActorVersion() const = 0;

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

    virtual outcome::result<TokenAmount> getBalance(
        const Address &address) const = 0;

    virtual TokenAmount getValueReceived() const = 0;

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
    virtual outcome::result<InvocationOutput> send(
        const Address &to_address,
        MethodNumber method_number,
        MethodParams params,
        const TokenAmount &value) = 0;

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
    virtual std::shared_ptr<IpfsDatastore> getIpfsDatastore() const = 0;

    /**
     * Get Message for actor invocation
     * @return message invoking current execution
     */
    virtual std::reference_wrapper<const UnsignedMessage> getMessage()
        const = 0;

    /// Try to charge gas or throw if there is not enoght gas
    virtual outcome::result<void> chargeGas(GasAmount amount) = 0;

    /// Get current actor state root CID
    virtual outcome::result<CID> getActorStateCid() const = 0;

    /// Update actor state CID
    virtual outcome::result<void> commit(const CID &new_state) = 0;

    /// Resolve address to id-address
    virtual outcome::result<boost::optional<Address>> tryResolveAddress(
        const Address &address) const = 0;

    outcome::result<Address> resolveAddress(const Address &address) const {
      OUTCOME_TRY(id, tryResolveAddress(address));
      if (id) {
        return *id;
      }
      return ERROR_TEXT("Runtime::resolveAddress: not found");
    }

    /// Verify signature
    virtual outcome::result<bool> verifySignature(
        const Signature &signature,
        const Address &address,
        gsl::span<const uint8_t> data) = 0;

    virtual outcome::result<bool> verifySignatureBytes(
        const Bytes &signature_bytes,
        const Address &address,
        gsl::span<const uint8_t> data) = 0;

    /// Verify PoSt
    virtual outcome::result<bool> verifyPoSt(
        const WindowPoStVerifyInfo &info) = 0;

    virtual outcome::result<BatchSealsOut> batchVerifySeals(
        const BatchSealsIn &batch) = 0;

    /// Compute unsealed sector cid
    virtual outcome::result<CID> computeUnsealedSectorCid(
        RegisteredSealProof type, const std::vector<PieceInfo> &pieces) = 0;

    /// Verify consensus fault
    virtual outcome::result<boost::optional<ConsensusFault>>
    verifyConsensusFault(const Bytes &block1,
                         const Bytes &block2,
                         const Bytes &extra) = 0;

    /// Return a hash of data
    virtual outcome::result<Blake2b256Hash> hashBlake2b(
        gsl::span<const uint8_t> data) = 0;

    /// Send typed method with typed params and result
    template <typename M>
    outcome::result<typename M::Result> sendM(const Address &address,
                                              const typename M::Params &params,
                                              const TokenAmount &value) {
      OUTCOME_TRY(params2, actor::encodeActorParams(params));
      OUTCOME_TRY(result, send(address, M::Number, params2, value));
      return actor::decodeActorReturn<typename M::Result>(result);
    }

    /// Send funds
    inline auto sendFunds(const Address &to, const TokenAmount &value) {
      return send(to, kSendMethodNumber, {}, value);
    }

    /// Send with typed result R
    template <typename R>
    outcome::result<R> sendR(const Address &to_address,
                             MethodNumber method_number,
                             const MethodParams &params,
                             const TokenAmount &value) {
      OUTCOME_TRY(result, send(to_address, method_number, params, value));
      return codec::cbor::decode<R>(result);
    }

    /// Send with typed params P and result R
    template <typename R, typename P>
    outcome::result<R> sendPR(const Address &to_address,
                              MethodNumber method_number,
                              const P &params,
                              const TokenAmount &value) {
      OUTCOME_TRY(params2, actor::encodeActorParams(params));
      return sendR<R>(to_address, method_number, MethodParams{params2}, value);
    }

    /// Send with typed params P
    template <typename P>
    outcome::result<InvocationOutput> sendP(const Address &to_address,
                                            MethodNumber method_number,
                                            const P &params,
                                            const TokenAmount &value) {
      OUTCOME_TRY(params2, actor::encodeActorParams(params));
      return send(to_address, method_number, MethodParams{params2}, value);
    }

    /// Get decoded current actor state. T must be Universal<State> type.
    template <typename T>
    outcome::result<T> getActorState() const {
      OUTCOME_TRY(head, getActorStateCid());
      return getCbor<T>(getIpfsDatastore(), head);
    }

    /**
     * Commit actor state
     * @param state - actor state Universal<State>
     * @return error in case of failure
     */
    template <typename T>
    outcome::result<void> commitState(const T &state) {
      OUTCOME_TRY(state_cid, setCbor(getIpfsDatastore(), state));
      OUTCOME_TRY(commit(state_cid));
      return outcome::success();
    }

    // TODO (a.chernyshov) make explicit
    // NOLINTNEXTLINE(google-explicit-constructor)
    inline operator std::shared_ptr<IpfsDatastore>() const {
      return getIpfsDatastore();
    }

    inline auto getCurrentBalance() const {
      return getBalance(getCurrentReceiver());
    }

    static inline outcome::result<void> validateArgument(bool assertion) {
      if (!assertion) {
        ABORT(VMExitCode::kErrIllegalArgument);
      }
      return outcome::success();
    }

    static inline outcome::result<void> requireState(bool assertion) {
      if (!assertion) {
        ABORT(VMExitCode::kErrIllegalState);
      }
      return outcome::success();
    }

    inline outcome::result<void> validateImmediateCallerIs(
        const Address &address) const {
      if (getImmediateCaller() == address) {
        return outcome::success();
      }
      ABORT(VMExitCode::kSysErrForbidden);
    }

    inline outcome::result<void> validateImmediateCallerIs(
        const std::vector<Address> &addresses) const {
      for (const auto &address : addresses) {
        if (getImmediateCaller() == address) {
          return outcome::success();
        }
      }
      ABORT(VMExitCode::kSysErrForbidden);
    }

    inline outcome::result<void> validateImmediateCallerType(
        const CID &expected_code) const {
      OUTCOME_TRY(actual_code, getActorCodeID(getImmediateCaller()));
      if (actual_code == expected_code) {
        return outcome::success();
      }
      ABORT(VMExitCode::kSysErrForbidden);
    }

    outcome::result<void> validateImmediateCallerIsSignable() const;

    outcome::result<void> validateImmediateCallerIsMiner() const;

    inline outcome::result<void> validateImmediateCallerIsCurrentReceiver()
        const {
      if (getImmediateCaller() == getCurrentReceiver()) {
        return outcome::success();
      }
      ABORT(VMExitCode::kSysErrForbidden);
    }

    inline outcome::result<void> vm_assert(bool condition) const {
      if (condition) {
        return outcome::success();
      }
      if (getNetworkVersion() <= NetworkVersion::kVersion3) {
        ABORT(VMExitCode::kOldErrActorFailure);
      }
      ABORT(VMExitCode::kSysErrReserved1);
    }

    inline outcome::result<Address> resolveOrCreate(const Address &address) {
      OUTCOME_TRY(id, tryResolveAddress(address));
      if (id) {
        return *id;
      }
      OUTCOME_TRY(sendFunds(address, 0));
      return resolveAddress(address);
    }
  };

}  // namespace fc::vm::runtime
