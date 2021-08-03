/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/runtime_impl.hpp"

#include "codec/cbor/cbor_codec.hpp"
#include "const.hpp"
#include "primitives/tipset/chain.hpp"
#include "proofs/impl/proof_engine_impl.hpp"
#include "storage/keystore/keystore.hpp"
#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/interpreter/interpreter.hpp"
#include "vm/runtime/env.hpp"
#include "vm/runtime/runtime_error.hpp"
#include "vm/toolchain/toolchain.hpp"
#include "vm/version/version.hpp"

namespace fc::vm::runtime {
  using actor::builtin::states::MinerActorStatePtr;
  using primitives::BigInt;
  using primitives::address::Protocol;
  using toolchain::Toolchain;

  RuntimeImpl::RuntimeImpl(std::shared_ptr<Execution> execution,
                           UnsignedMessage message,
                           const Address &caller_id)
      : execution_{std::move(execution)},
        message_{std::move(message)},
        caller_id{caller_id},
        proofs_(std::make_shared<proofs::ProofEngineImpl>()) {}

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
    return Toolchain::getActorVersionForNetwork(getNetworkVersion());
  }

  outcome::result<Randomness> RuntimeImpl::getRandomnessFromTickets(
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    return execution_->env->env_context.randomness->getRandomnessFromTickets(
        execution_->env->ts_branch, tag, epoch, seed);
  }

  outcome::result<Randomness> RuntimeImpl::getRandomnessFromBeacon(
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    return execution_->env->env_context.randomness->getRandomnessFromBeacon(
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
    if (auto circulating{execution_->env->env_context.circulating}) {
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

  outcome::result<CID> RuntimeImpl::getActorStateCid() const {
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
    return storage::keystore::kDefaultKeystore->verify(
        account, data, signature);
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

    return storage::keystore::kDefaultKeystore->verify(
        account, data, signature.value());
  }

  outcome::result<bool> RuntimeImpl::verifyPoSt(
      const WindowPoStVerifyInfo &info) {
    OUTCOME_TRY(chargeGas(execution_->env->pricelist.onVerifyPost(info)));
    WindowPoStVerifyInfo preprocess_info = info;
    preprocess_info.randomness[31] &= 0x3f;
    return proofs_->verifyWindowPoSt(preprocess_info);
  }

  outcome::result<BatchSealsOut> RuntimeImpl::batchVerifySeals(
      const BatchSealsIn &batch) {
    BatchSealsOut res;
    for (auto &[miner, seals] : batch) {
      std::vector<SectorNumber> successful;
      successful.reserve(seals.size());
      std::set<SectorNumber> seen;
      for (auto &seal : seals) {
        auto verified{proofs_->verifySeal(seal)};
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
    return proofs_->generateUnsealedCID(type, pieces, true);
  }

  // TODO: reuse in block validation
  outcome::result<bool> checkBlockSignature(const BlockHeader &block,
                                            const Address &worker) {
    if (!block.block_sig) {
      return false;
    }
    auto block2{block};
    block2.block_sig.reset();
    OUTCOME_TRY(data, codec::cbor::encode(block2));
    return storage::keystore::kDefaultKeystore->verify(
        worker, data, *block.block_sig);
  }

  // TODO: reuse
  template <typename T>
  inline bool has(const std::vector<T> &xs, const T &x) {
    return std::find(xs.begin(), xs.end(), x) != xs.end();
  }

  // TODO: reuse in slash filter
  inline bool isNearOrange(ChainEpoch epoch) {
    using actor::builtin::types::miner::kChainFinality;
    return epoch > kUpgradeOrangeHeight - kChainFinality
           && epoch < kUpgradeOrangeHeight + kChainFinality;
  }

  outcome::result<boost::optional<ConsensusFault>>
  RuntimeImpl::verifyConsensusFault(const Buffer &block1,
                                    const Buffer &block2,
                                    const Buffer &extra) {
    using common::getCidOf;
    OUTCOME_TRY(chargeGas(execution_->env->pricelist.onVerifyConsensusFault()));
    if (block1 == block2) {
      return boost::none;
    }
    auto _blockA{codec::cbor::decode<BlockHeader>(block1)};
    if (!_blockA) {
      return boost::none;
    }
    auto &blockA{_blockA.value()};
    auto _blockB{codec::cbor::decode<BlockHeader>(block2)};
    if (!_blockB) {
      return boost::none;
    }
    auto &blockB{_blockB.value()};
    ConsensusFault fault;
    fault.target = blockA.miner;
    fault.epoch = blockB.height;
    boost::optional<ConsensusFaultType> type;
    if (isNearOrange(blockA.height) || isNearOrange(blockB.height)
        || blockA.miner != blockB.miner || blockA.height > blockB.height) {
      return boost::none;
    }
    if (blockA.height == blockB.height) {
      type = ConsensusFaultType::DoubleForkMining;
    } else if (blockA.parents == blockB.parents) {
      type = ConsensusFaultType::TimeOffsetMining;
    }
    if (!extra.empty()) {
      if (auto _blockC{codec::cbor::decode<BlockHeader>(extra)}) {
        auto &blockC{_blockC.value()};
        if (blockA.parents == blockC.parents && blockA.height == blockC.height
            && has(blockB.parents, CbCid::hash(extra))
            && !has(blockB.parents, CbCid::hash(block1))) {
          type = ConsensusFaultType::ParentGrinding;
        }
      } else {
        return boost::none;
      }
    }
    if (type) {
      auto verify2{[&](const BlockHeader &block) -> outcome::result<bool> {
        if (getNetworkVersion() >= NetworkVersion::kVersion7
            && static_cast<ChainEpoch>(block.height)
                   < getCurrentEpoch()
                         - vm::actor::builtin::types::miner::kChainFinality) {
          return false;
        }
        auto &env{execution_->env};

        std::shared_lock ts_lock{*env->env_context.ts_branches_mutex};
        OUTCOME_TRY(it, find(env->ts_branch, getCurrentEpoch()));
        OUTCOME_TRYA(it, getLookbackTipSetForRound(it, block.height));
        OUTCOME_TRYA(it, find(env->ts_branch, it.second->first + 1, false));
        OUTCOME_TRY(child_ts,
                    env->env_context.ts_load->lazyLoad(it.second->second));
        ts_lock.unlock();

        const StateTreeImpl state_tree{env->ipld,
                                       child_ts->getParentStateRoot()};
        auto &ipld{execution_->charging_ipld};

        OUTCOME_TRY(actor, state_tree.get(block.miner));

        OUTCOME_TRY(state, getCbor<MinerActorStatePtr>(ipld, actor.head));
        OUTCOME_TRY(miner_info, state->getInfo());

        OUTCOME_TRY(
            key, resolveKey(*execution_->state_tree, ipld, miner_info->worker));
        return checkBlockSignature(block, key);
      }};
      auto verify{[&](const BlockHeader &block) -> outcome::result<bool> {
        if (auto _ok{verify2(block)}) {
          return _ok;
        } else if (isAbortExitCode(_ok.error())) {
          return _ok;
        } else {
          return false;
        }
      }};
      OUTCOME_TRY(okA, verify(blockA));
      if (!okA) {
        return boost::none;
      }
      OUTCOME_TRY(okB, verify(blockB));
      if (!okB) {
        return boost::none;
      }
      fault.type = *type;
      return fault;
    }
    return boost::none;
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
