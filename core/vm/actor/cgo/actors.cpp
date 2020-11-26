/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/cgo/actors.hpp"

#include "crypto/blake2/blake2b160.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"
#include "proofs/proofs.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/cgo/c_actors.h"
#include "vm/actor/cgo/go_actors.h"
#include "vm/dvm/dvm.hpp"
#include "vm/runtime/env.hpp"
#include "vm/runtime/impl/runtime_impl.hpp"

#define RUNTIME_METHOD(name)                                         \
  void rt_##name(Runtime &, CborDecodeStream &, CborEncodeStream &); \
  CBOR_METHOD(name) {                                                \
    rt_##name(runtimes.at(arg.get<size_t>()), arg, ret);             \
  }                                                                  \
  void rt_##name(Runtime &rt, CborDecodeStream &arg, CborEncodeStream &ret)

namespace fc::vm::actor::cgo {
  using builtin::v0::account::AccountActorState;
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;
  using crypto::signature::Signature;
  using primitives::ChainEpoch;
  using primitives::GasAmount;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PieceInfo;
  using primitives::sector::SealVerifyInfo;
  using primitives::sector::WindowPoStVerifyInfo;
  using runtime::resolveKey;
  using runtime::RuntimeImpl;
  using storage::hamt::HamtError;
  using vm::version::getNetworkVersion;

  void config(const StoragePower &min_verified_deal_size,
              const StoragePower &consensus_miner_min_power,
              const std::vector<RegisteredProof> &supported_proofs) {
    CborEncodeStream arg;
    arg << min_verified_deal_size << consensus_miner_min_power;
    arg << supported_proofs.size();
    for (auto proof : supported_proofs) {
      arg << proof;
    }
    cgoCall<cgoActorsConfig>(arg);
  }

  constexpr auto kFatal{VMExitCode::kFatal};
  constexpr auto kOk{VMExitCode::kOk};

  static std::map<size_t, RuntimeImpl> runtimes;
  static size_t next_runtime{0};

  static storage::keystore::InMemoryKeyStore keystore{
      std::make_shared<crypto::bls::BlsProviderImpl>(),
      std::make_shared<crypto::secp256k1::Secp256k1ProviderImpl>()};

  outcome::result<Buffer> invoke(const std::shared_ptr<Execution> &exec,
                                 const UnsignedMessage &message,
                                 const CID &code,
                                 size_t method,
                                 BytesIn params) {
    CborEncodeStream arg;
    auto id{next_runtime++};  // TODO: mod
    auto runtime =
        RuntimeImpl(exec, exec->env->randomness, message, message.from);
    auto version{runtime.getNetworkVersion()};
    arg << id << version << message.from << message.to << exec->env->epoch
        << message.value << code << method << params;
    runtimes.emplace(id, runtime);
    auto ret{cgoCall<cgoActorsInvoke>(arg)};
    runtimes.erase(id);
    auto exit{ret.get<VMExitCode>()};
    if (exit != kOk) {
      return exit;
    }
    return ret.get<Buffer>();
  }

  template <typename T>
  inline auto chargeFatal(CborEncodeStream &ret, const outcome::result<T> &r) {
    if (!r) {
      if (r.error() == VMExitCode::kSysErrOutOfGas) {
        ret << VMExitCode::kSysErrOutOfGas;
      } else {
        ret << kFatal;
      }
      return true;
    }
    return false;
  }

  inline auto charge(CborEncodeStream &ret, Runtime &rt, GasAmount gas) {
    return !chargeFatal(ret, rt.execution()->chargeGas(gas));
  }

  inline boost::optional<Buffer> ipldGet(CborEncodeStream &ret,
                                         Runtime &rt,
                                         const CID &cid) {
    if (auto r{rt.execution()->charging_ipld->get(cid)}) {
      return std::move(r.value());
    } else {
      chargeFatal(ret, r);
    }
    return {};
  }

  inline boost::optional<CID> ipldPut(CborEncodeStream &ret,
                                      Runtime &rt,
                                      BytesIn value) {
    OUTCOME_EXCEPT(cid, common::getCidOf(value));
    if (auto r{rt.execution()->charging_ipld->set(cid, Buffer{value})}) {
      return std::move(cid);
    } else {
      chargeFatal(ret, r);
    }
    return {};
  }

  RUNTIME_METHOD(gocRtIpldGet) {
    if (auto value{ipldGet(ret, rt, arg.get<CID>())}) {
      ret << kOk << *value;
    }
  }

  RUNTIME_METHOD(gocRtIpldPut) {
    auto buf = arg.get<Buffer>();
    if (auto cid{ipldPut(ret, rt, buf)}) {
      ret << kOk << *cid;
    }
  }

  RUNTIME_METHOD(gocRtCharge) {
    if (charge(ret, rt, arg.get<GasAmount>())) {
      ret << kOk;
    }
  }

  RUNTIME_METHOD(gocRtRandomnessFromTickets) {
    auto tag{arg.get<DomainSeparationTag>()};
    auto round{arg.get<ChainEpoch>()};
    auto seed{arg.get<Buffer>()};
    auto r = rt.getRandomnessFromTickets(tag, round, seed);
    if (!r) {
      ret << kFatal;
    } else {
      ret << kOk << r.value();
    }
  }

  RUNTIME_METHOD(gocRtRandomnessFromBeacon) {
    auto tag{arg.get<DomainSeparationTag>()};
    auto round{arg.get<ChainEpoch>()};
    auto seed{arg.get<Buffer>()};
    auto r = rt.getRandomnessFromBeacon(tag, round, seed);
    if (!r) {
      ret << kFatal;
    } else {
      ret << kOk << r.value();
    }
  }

  RUNTIME_METHOD(gocRtBlake) {
    if (charge(ret, rt, rt.execution()->env->pricelist.onHashing())) {
      ret << kOk << crypto::blake2b::blake2b_256(arg.get<Buffer>());
    }
  }

  RUNTIME_METHOD(gocRtVerifyPost) {
    auto info{arg.get<WindowPoStVerifyInfo>()};
    if (charge(ret, rt, rt.execution()->env->pricelist.onVerifyPost(info))) {
      info.randomness[31] &= 0x3f;
      auto r{proofs::Proofs::verifyWindowPoSt(info)};
      ret << kOk << (r && r.value());
    }
  }

  RUNTIME_METHOD(gocRtVerifySeals) {
    auto n{arg.get<size_t>()};
    ret << kOk;
    for (auto i{0u}; i < n; ++i) {
      auto r{proofs::Proofs::verifySeal(arg.get<SealVerifyInfo>())};
      ret << (r && r.value());
    }
  }

  RUNTIME_METHOD(gocRtActorId) {
    auto r{rt.execution()->state_tree->lookupId(arg.get<Address>())};
    if (!r) {
      if (r.error() == HamtError::kNotFound) {
        ret << kOk << false;
      } else {
        ret << kFatal;
      }
    } else {
      ret << kOk << true << r.value();
    }
  }

  RUNTIME_METHOD(gocRtSend) {
    UnsignedMessage m;
    m.from = rt.getMessage().get().to;
    m.to = arg.get<Address>();
    m.method = arg.get<uint64_t>();
    m.params = arg.get<Buffer>();
    m.value = arg.get<TokenAmount>();
    auto r{rt.execution()->sendWithRevert(m)};
    if (!r) {
      auto &e{r.error()};
      if (!isVMExitCode(e) || e == kFatal) {
        ret << kFatal;
      } else {
        ret << kOk << e.value();

        dvm::onReceipt({VMExitCode{e.value()}, {}, rt.execution()->gas_used});
      }
    } else {
      ret << kOk << kOk << r.value();

      dvm::onReceipt(
          {VMExitCode::kOk, std::move(r.value()), rt.execution()->gas_used});
    }
  }

  RUNTIME_METHOD(gocRtVerifySig) {
    auto _sig{arg.get<Buffer>()};
    auto bls{!_sig.empty() && _sig[0] == crypto::signature::BLS};
    if (charge(
            ret, rt, rt.execution()->env->pricelist.onVerifySignature(bls))) {
      auto address{arg.get<Address>()};
      auto ok{address.isKeyType()};
      if (address.isId()) {
        // resolve id address to key address
        if (auto actor{rt.execution()->state_tree->get(address)}) {
          if (auto state{ipldGet(ret, rt, actor.value().head)}) {
            if (auto account{
                    codec::cbor::decode<AccountActorState>(state.value())}) {
              address = account.value().address;
              ok = true;
            }
          } else {
            return;
          }
        }
      }
      if (ok) {
        if (auto sig{Signature::fromBytes(_sig)}) {
          auto input{arg.get<Buffer>()};
          auto r{keystore.verify(address, input, sig.value())};
          ok = r && r.value();
        } else {
          ok = false;
        }
      }
      ret << kOk << ok;
    }
  }

  RUNTIME_METHOD(gocRtCommD) {
    if (charge(ret,
               rt,
               rt.execution()->env->pricelist.onComputeUnsealedSectorCid())) {
      auto type{arg.get<RegisteredProof>()};
      auto pieces{arg.get<std::vector<PieceInfo>>()};
      if (auto r{proofs::Proofs::generateUnsealedCID(type, pieces, true)}) {
        ret << kOk << true << r.value();
      } else {
        ret << kOk << false;
      }
    }
  }

  RUNTIME_METHOD(gocRtNewAddress) {
    auto &exec{*rt.execution()};
    if (auto _key{resolveKey(*exec.state_tree, exec.origin)}) {
      if (auto _seed{codec::cbor::encode(_key.value())}) {
        auto &seed{_seed.value()};
        seed.putUint64(exec.origin_nonce);
        seed.putUint64(exec.actors_created);
        ++exec.actors_created;
        ret << kOk << Address::makeActorExec(seed);
      } else {
        ret << kFatal;
      }
    } else {
      ret << kFatal;
    }
  }

  RUNTIME_METHOD(gocRtCreateActor) {
    auto code{arg.get<CID>()};
    auto address{arg.get<Address>()};
    if (!actor::isBuiltinActor(code) || actor::isSingletonActor(code)
        || rt.execution()->state_tree->get(address)) {
      ret << VMExitCode::kSysErrorIllegalArgument;
    } else if (charge(
                   ret, rt, rt.execution()->env->pricelist.onCreateActor())) {
      if (rt.execution()->state_tree->set(
              address, {code, actor::kEmptyObjectCid, 0, 0})) {
        ret << kOk;
      } else {
        ret << kFatal;
      }
    }
  }

  RUNTIME_METHOD(gocRtActorCode) {
    if (auto _actor{rt.execution()->state_tree->get(arg.get<Address>())}) {
      ret << kOk << true << _actor.value().code;
    } else {
      if (_actor.error() == HamtError::kNotFound) {
        ret << kOk << false;
      } else {
        ret << kFatal;
      }
    }
  }

  RUNTIME_METHOD(gocRtActorBalance) {
    if (auto balance{rt.getBalance(rt.getMessage().get().to)}) {
      ret << kOk << balance.value();
    } else {
      ret << kFatal;
    }
  }

  RUNTIME_METHOD(gocRtStateGet) {
    if (auto _actor{
            rt.execution()->state_tree->get(rt.getMessage().get().to)}) {
      auto &head{_actor.value().head};
      if (auto state{ipldGet(ret, rt, head)}) {
        ret << kOk << true << *state;
        if (arg.get<bool>()) {
          ret << head;
        }
      }
    } else {
      ret << kOk << false;
    }
  }

  RUNTIME_METHOD(gocRtStateCommit) {
    if (auto cid{ipldPut(ret, rt, arg.get<Buffer>())}) {
      if (auto _actor{
              rt.execution()->state_tree->get(rt.getMessage().get().to)}) {
        auto &actor{_actor.value()};
        if (actor.head != arg.get<CID>()) {
          ret << kFatal;
        } else {
          actor.head = *cid;
          if (rt.execution()->state_tree->set(rt.getMessage().get().to,
                                              actor)) {
            ret << kOk;
          } else {
            ret << kFatal;
          }
        }
      } else {
        ret << kFatal;
      }
    }
  }

  RUNTIME_METHOD(gocRtDeleteActor) {
    if (charge(ret, rt, rt.execution()->env->pricelist.onDeleteActor())) {
      auto to{arg.get<Address>()};
      auto &state{*rt.execution()->state_tree};
      if (auto _actor{state.get(rt.getMessage().get().to)}) {
        auto &balance{_actor.value().balance};
        auto transfer{[&]() -> outcome::result<void> {
          OUTCOME_TRY(from_id, state.lookupId(rt.getMessage().get().to));
          OUTCOME_TRY(to_id, state.lookupId(to));
          if (from_id != to_id) {
            OUTCOME_TRY(from_actor, state.get(from_id));
            OUTCOME_TRY(to_actor, state.get(to_id));
            from_actor.balance -= balance;
            to_actor.balance += balance;
            OUTCOME_TRY(state.set(from_id, from_actor));
            OUTCOME_TRY(state.set(to_id, to_actor));
          }
          return outcome::success();
        }};
        if (balance.is_zero() || transfer()) {
          if (state.remove(rt.getMessage().get().to)) {
            ret << kOk;
          } else {
            ret << kFatal;
          }
        } else {
          ret << kFatal;
        }
      } else {
        if (_actor.error() == HamtError::kNotFound) {
          ret << VMExitCode::kSysErrorIllegalActor;
        } else {
          ret << kFatal;
        }
      }
    }
  }
}  // namespace fc::vm::actor::cgo
