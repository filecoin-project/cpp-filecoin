/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/runtime_impl.hpp"

#include "codec/cbor/cbor.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"
#include "proofs/proofs.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/runtime/env.hpp"
#include "vm/runtime/runtime_error.hpp"
#include "vm/toolchain/toolchain.hpp"
#include "vm/version.hpp"

namespace fc::vm::runtime {
  using fc::primitives::BigInt;
  using fc::primitives::address::Protocol;
  using toolchain::Toolchain;

  RuntimeImpl::RuntimeImpl(std::shared_ptr<Execution> execution,
                           UnsignedMessage message,
                           const Address &caller_id)
      : execution_{std::move(execution)},
        message_{std::move(message)},
        caller_id{caller_id} {}

  std::shared_ptr<Execution> RuntimeImpl::execution() const {
    return execution_;
  }

  NetworkVersion RuntimeImpl::getNetworkVersion() const {
    return version::getNetworkVersion(getCurrentEpoch());
  }

  ChainEpoch RuntimeImpl::getCurrentEpoch() const {
    return execution_->env->epoch;
  }

  ActorVersion RuntimeImpl::getActorVersion() const {
    return actor::getActorVersionForNetwork(getNetworkVersion());
  }

  outcome::result<Randomness> RuntimeImpl::getRandomnessFromTickets(
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    return execution_->env->randomness->getRandomnessFromTickets(
        execution_->env->ts_branch, tag, epoch, seed);
  }

  outcome::result<Randomness> RuntimeImpl::getRandomnessFromBeacon(
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    return execution_->env->randomness->getRandomnessFromBeacon(
        execution_->env->ts_branch, tag, epoch, seed);
  }

  Address RuntimeImpl::getImmediateCaller() const {
    return caller_id;
  }

  Address RuntimeImpl::getCurrentReceiver() const {
    return message_.to;
  }

  outcome::result<BigInt> RuntimeImpl::getBalance(
      const Address &address) const {
    OUTCOME_TRY(actor_state, execution_->state_tree->tryGet(address));
    if (!actor_state) {
      return BigInt(0);
    }
    return actor_state.value().balance;
  }

  BigInt RuntimeImpl::getValueReceived() const {
    return message_.value;
  }

  outcome::result<CodeId> RuntimeImpl::getActorCodeID(
      const Address &address) const {
    OUTCOME_TRY(actor_state, execution_->state_tree->get(address));
    return actor_state.code;
  }

  outcome::result<InvocationOutput> RuntimeImpl::send(
      Address to_address,
      MethodNumber method_number,
      MethodParams params,
      BigInt value) {
    return execution_->sendWithRevert(
        {to_address, message_.to, {}, value, {}, {}, method_number, params});
  }

  outcome::result<Address> RuntimeImpl::createNewActorAddress() {
    OUTCOME_TRY(caller_address,
                resolveKey(*execution_->state_tree,
                           execution_->env->ipld,
                           execution()->origin));
    OUTCOME_TRY(encoded_address, codec::cbor::encode(caller_address));
    auto actor_address{Address::makeActorExec(
        encoded_address.putUint64(execution()->origin_nonce)
            .putUint64(execution_->actors_created))};

    ++execution_->actors_created;
    return actor_address;
  }

  outcome::result<void> RuntimeImpl::createActor(const Address &address,
                                                 const Actor &actor) {
    const auto address_matcher =
        Toolchain::createAddressMatcher(getActorVersion());
    if (!address_matcher->isBuiltinActor(actor.code)
        || address_matcher->isSingletonActor(actor.code)) {
      ABORT(VMExitCode::kSysErrIllegalArgument);
    }

    const auto has = execution_->state_tree->get(address);
    if (!has.has_error()) {
      ABORT(VMExitCode::kSysErrIllegalArgument);
    }

    OUTCOME_TRY(chargeGas(execution_->env->pricelist.onCreateActor()));
    OUTCOME_TRY(execution_->state_tree->set(address, actor));
    return outcome::success();
  }

  outcome::result<void> RuntimeImpl::deleteActor(const Address &address) {
    OUTCOME_TRY(chargeGas(execution_->env->pricelist.onDeleteActor()));
    auto &state{*execution()->state_tree};
    if (auto _actor{state.tryGet(getCurrentReceiver())}) {
      if (auto &actor{_actor.value()}) {
        const auto &balance{actor->balance};
        if (balance.is_zero()
            || transfer(getCurrentReceiver(), address, balance)) {
          if (state.remove(getCurrentReceiver())) {
            return outcome::success();
          }
        }
      }
    }
    return VMExitCode::kSysErrIllegalActor;
  }

  outcome::result<void> RuntimeImpl::transfer(const Address &debitFrom,
                                              const Address &creditTo,
                                              const TokenAmount &amount) {
    if (amount < 0) {
      return VMExitCode::kSysErrForbidden;
    }

    auto &state{*execution()->state_tree};

    OUTCOME_TRY(from_id, state.lookupId(debitFrom));
    OUTCOME_TRY(to_id, state.lookupId(creditTo));
    if (from_id != to_id) {
      OUTCOME_TRY(from_actor, state.get(from_id));
      OUTCOME_TRY(to_actor, state.get(to_id));

      if (from_actor.balance < amount) {
        return VMExitCode::kSysErrInsufficientFunds;
      }

      from_actor.balance -= amount;
      to_actor.balance += amount;
      OUTCOME_TRY(state.set(from_id, from_actor));
      OUTCOME_TRY(state.set(to_id, to_actor));
    }

    return outcome::success();
  }

  outcome::result<TokenAmount> RuntimeImpl::getTotalFilCirculationSupply()
      const {
    if (auto circulating{execution_->env->circulating}) {
      return circulating->circulating(execution_->state_tree,
                                      getCurrentEpoch());
    }
    return 0;
  }

  std::shared_ptr<IpfsDatastore> RuntimeImpl::getIpfsDatastore() const {
    return execution_->charging_ipld;
  }

  std::reference_wrapper<const UnsignedMessage> RuntimeImpl::getMessage()
      const {
    return message_;
  }

  outcome::result<CID> RuntimeImpl::getCurrentActorState() const {
    OUTCOME_TRY(actor, execution_->state_tree->get(getCurrentReceiver()));
    return actor.head;
  }

  outcome::result<void> RuntimeImpl::commit(const CID &new_state) {
    OUTCOME_TRY(actor, execution_->state_tree->get(getCurrentReceiver()));
    actor.head = new_state;
    OUTCOME_TRY(execution_->state_tree->set(getCurrentReceiver(), actor));
    return outcome::success();
  }

  outcome::result<boost::optional<Address>> RuntimeImpl::tryResolveAddress(
      const Address &address) const {
    return execution_->state_tree->tryLookupId(address);
  }

  outcome::result<bool> RuntimeImpl::verifySignature(
      const Signature &signature,
      const Address &address,
      gsl::span<const uint8_t> data) {
    OUTCOME_TRY(chargeGas(
        execution_->env->pricelist.onVerifySignature(signature.isBls())));
    OUTCOME_TRY(
        account,
        resolveKey(
            *execution_->state_tree, execution_->charging_ipld, address));
    return storage::keystore::InMemoryKeyStore{
        std::make_shared<crypto::bls::BlsProviderImpl>(),
        std::make_shared<crypto::secp256k1::Secp256k1ProviderImpl>()}
        .verify(account, data, signature);
  }

  outcome::result<bool> RuntimeImpl::verifySignatureBytes(
      const Buffer &signature_bytes,
      const Address &address,
      gsl::span<const uint8_t> data) {
    const auto bls = Signature::isBls(signature_bytes);
    if (bls.has_error()) {
      return false;
    }
    OUTCOME_TRY(
        chargeGas(execution_->env->pricelist.onVerifySignature(bls.value())));
    OUTCOME_TRY(
        account,
        resolveKey(
            *execution_->state_tree, execution_->charging_ipld, address));

    const auto signature = Signature::fromBytes(signature_bytes);
    if (!signature) {
      return false;
    }

    return storage::keystore::InMemoryKeyStore{
        std::make_shared<crypto::bls::BlsProviderImpl>(),
        std::make_shared<crypto::secp256k1::Secp256k1ProviderImpl>()}
        .verify(account, data, signature.value());
  }

  outcome::result<bool> RuntimeImpl::verifyPoSt(
      const WindowPoStVerifyInfo &info) {
    OUTCOME_TRY(chargeGas(execution_->env->pricelist.onVerifyPost(info)));
    WindowPoStVerifyInfo preprocess_info = info;
    preprocess_info.randomness[31] &= 0x3f;
    return proofs::Proofs::verifyWindowPoSt(preprocess_info);
  }

  outcome::result<BatchSealsOut> RuntimeImpl::batchVerifySeals(
      const BatchSealsIn &batch) {
    BatchSealsOut res;
    for (auto &[miner, seals] : batch) {
      std::vector<SectorNumber> successful;
      successful.reserve(seals.size());
      std::set<SectorNumber> seen;
      for (auto &seal : seals) {
        auto verified{proofs::Proofs::verifySeal(seal)};
        if (verified && verified.value()
            && seen.insert(seal.sector.sector).second) {
          successful.push_back(seal.sector.sector);
        }
      }
      res.emplace_back(miner, std::move(successful));
    }
    return res;
  }

  outcome::result<CID> RuntimeImpl::computeUnsealedSectorCid(
      RegisteredSealProof type, const std::vector<PieceInfo> &pieces) {
    OUTCOME_TRY(
        chargeGas(execution_->env->pricelist.onComputeUnsealedSectorCid()));
    return proofs::Proofs::generateUnsealedCID(type, pieces, true);
  }

  outcome::result<ConsensusFault> RuntimeImpl::verifyConsensusFault(
      const Buffer &block1, const Buffer &block2, const Buffer &extra) {
    // TODO(a.chernyshov): implement
    return RuntimeError::kUnknown;
  }

  outcome::result<Blake2b256Hash> RuntimeImpl::hashBlake2b(
      gsl::span<const uint8_t> data) {
    OUTCOME_TRY(chargeGas(execution_->env->pricelist.onHashing()));
    return crypto::blake2b::blake2b_256(data);
  }

  outcome::result<void> RuntimeImpl::chargeGas(GasAmount amount) {
    return execution_->chargeGas(amount);
  }
}  // namespace fc::vm::runtime
