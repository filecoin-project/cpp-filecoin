/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/fvm/fvm.hpp"

#include <filecoin-ffi/filcrypto.h>
#include <shared_mutex>

#include "cbor_blake/ipld_any.hpp"
#include "cbor_blake/ipld_version.hpp"
#include "codec/cbor/light_reader/cid.hpp"
#include "common/ffi.hpp"
#include "common/logger.hpp"
#include "vm/fvm/ubig128.hpp"
#include "vm/runtime/circulating.hpp"
#include "vm/runtime/consensus_fault.hpp"
#include "vm/runtime/runtime_randomness.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

#define FFI_TRY(name)                                                          \
  do {                                                                         \
    if (res->status_code != FCPResponseStatus::FCPResponseStatus_FCPNoError) { \
      spdlog::error(                                                           \
          "{} status={} message={}", #name, res->status_code, res->error_msg); \
      return ERROR_TEXT(#name);                                                \
    }                                                                          \
  } while (0)

#define FVM_MACHINE()                               \
  std::shared_lock lock{machines_mutex};            \
  const auto machine_it{machines.find(machine_id)}; \
  if (machine_it == machines.end()) {               \
    return FvmError::kInvalidHandle;                \
  }                                                 \
  const auto machine{machine_it->second.lock()};    \
  if (!machine) {                                   \
    return FvmError::kInvalidHandle;                \
  }

namespace fc::vm::fvm {
  using codec::cbor::light_reader::readCborBlake;

  inline void initCallbacks();

  std::shared_mutex machines_mutex;
  std::atomic<FvmMachineId> next_machine_id{0};
  std::map<FvmMachineId, std::weak_ptr<FvmMachine>> machines;

  outcome::result<std::shared_ptr<FvmMachine>> FvmMachine::make(
      EnvironmentContext envx,
      TsBranchPtr ts_branch,
      const TokenAmount &base_fee,
      const CID &state,
      ChainEpoch epoch) {
    initCallbacks();
    envx.ipld = withVersion(envx.ipld, epoch);
    const auto network{version::getNetworkVersion(epoch)};
    TokenAmount circ;
    if (envx.circulating) {
      if (network >= NetworkVersion::kVersion15) {
        const auto tree{
            std::make_shared<state::StateTreeImpl>(envx.ipld, state)};
        OUTCOME_TRYA(circ, envx.circulating->circulating(tree, epoch));
      } else {
        OUTCOME_TRYA(circ, envx.circulating->vested(epoch));
      }
    }

    const auto id{++next_machine_id};
    auto machine{std::make_shared<FvmMachine>()};
    machine->machine_id = id;
    machine->envx = std::move(envx);
    machine->ts_branch = std::move(ts_branch);
    machine->epoch = epoch;
    machine->base_state = state;
    std::unique_lock lock{machines_mutex};
    machines.emplace(id, machine);
    lock.unlock();

    OUTCOME_TRY(base_fee_128, Ubig128::fromBig(base_fee));
    OUTCOME_TRY(circ_128, Ubig128::fromBig(circ));
    const auto state_bytes{state.toBytes().value()};
    const auto res{
        common::ffi::wrap(fil_create_fvm_machine(fil_FvmRegisteredVersion_V1,
                                                 epoch,
                                                 base_fee_128.high,
                                                 base_fee_128.low,
                                                 circ_128.high,
                                                 circ_128.low,
                                                 gsl::narrow<uint64_t>(network),
                                                 state_bytes.data(),
                                                 state_bytes.size(),
                                                 id,
                                                 id),
                          fil_destroy_create_fvm_machine_response)};
    machine->executor = {res->executor, fil_drop_fvm_machine};
    FFI_TRY(fil_create_fvm_machine);

    return machine;
  }

  FvmMachine::~FvmMachine() {
    std::unique_lock lock{machines_mutex};
    machines.erase(machine_id);
  }

  outcome::result<ApplyRet> FvmMachine::applyMessage(
      const UnsignedMessage &message, size_t size) {
    OUTCOME_TRY(message_bytes, codec::cbor::encode(message));
    const auto res{
        common::ffi::wrap(fil_fvm_machine_execute_message(executor.get(),
                                                          message_bytes.data(),
                                                          message_bytes.size(),
                                                          size,
                                                          size == 0 ? 1 : 0),
                          fil_destroy_fvm_machine_execute_response)};
    FFI_TRY(fil_fvm_machine_execute_message);
    return ApplyRet{
        {
            gsl::narrow<VMExitCode>(res->exit_code),
            {res->return_ptr, res->return_ptr + res->return_len},
            gsl::narrow<GasAmount>(res->gas_used),
        },
        Ubig128{res->penalty_hi, res->penalty_lo}.big(),
        Ubig128{res->miner_tip_hi, res->miner_tip_lo}.big(),
    };
  }

  outcome::result<MessageReceipt> FvmMachine::applyImplicitMessage(
      const UnsignedMessage &message) {
    OUTCOME_TRY(apply, applyMessage(message, 0));
    return apply.receipt;
  }

  outcome::result<CID> FvmMachine::flush() {
    const auto res{common::ffi::wrap(fil_fvm_machine_flush(executor.get()),
                                     fil_destroy_fvm_machine_flush_response)};
    FFI_TRY(fil_fvm_machine_flush);
    auto cid_bytes{gsl::make_span(res->state_root_ptr, res->state_root_len)};
    const CbCid *cid{};
    if (!readCborBlake(cid, cid_bytes) || !cid_bytes.empty()) {
      return ERROR_TEXT("fil_fvm_machine_flush: not CbCid");
    }
    return CID{*cid};
  }

  FvmError callbackIpldGet(FvmMachineId machine_id,
                           BytesIn key,
                           Bytes *out_value) {
    FVM_MACHINE();
    const CbCid *cid{};
    if (!readCborBlake(cid, key) || !key.empty()) {
      return FvmError::kInvalidArgument;
    }
    const AnyAsCbIpld ipld{machine->envx.ipld};
    if (ipld.get(*cid, out_value)) {
      return out_value ? FvmError::kOk : FvmError::kIpldHas;
    }
    return out_value ? FvmError::kNotFound : FvmError::kOk;
  }

  FvmError callbackIpldPut(FvmMachineId machine_id,
                           BytesIn key,
                           BytesIn value) {
    FVM_MACHINE();
    const CbCid *cid{};
    if (!readCborBlake(cid, key) || !key.empty()) {
      return FvmError::kInvalidArgument;
    }
    const auto expected_cid{CbCid::hash(value)};
    if (*cid != expected_cid) {
      return FvmError::kInvalidArgument;
    }
    AnyAsCbIpld ipld{machine->envx.ipld};
    ipld.put(*cid, value);
    return FvmError::kOk;
  }

  FvmError callbackIpldPutMany(FvmMachineId machine_id,
                               gsl::span<const int32_t> sizes,
                               BytesIn keys_values) {
    FVM_MACHINE();
    AnyAsCbIpld ipld{machine->envx.ipld};
    for (const auto &size : sizes) {
      BytesIn value;
      if (!codec::read(value, keys_values, size)) {
        return FvmError::kInvalidArgument;
      }
      const CbCid *cid{};
      if (!readCborBlake(cid, value)) {
        return FvmError::kInvalidArgument;
      }
      const auto expected_cid{CbCid::hash(value)};
      if (*cid != expected_cid) {
        return FvmError::kInvalidArgument;
      }
      ipld.put(*cid, value);
    }
    if (!keys_values.empty()) {
      return FvmError::kInvalidArgument;
    }
    return FvmError::kOk;
  }

  FvmError callbackRand(FvmMachineId machine_id,
                        bool beacon,
                        DomainSeparationTag tag,
                        ChainEpoch epoch,
                        BytesIn seed,
                        Randomness &out_randomness) {
    FVM_MACHINE();
    using runtime::RuntimeRandomness;
    const auto randomness{
        ((*machine->envx.randomness)
         .*(beacon ? &RuntimeRandomness::getRandomnessFromBeacon
                   : &RuntimeRandomness::getRandomnessFromTickets))(
            machine->ts_branch, tag, epoch, seed)};
    if (!randomness) {
      return FvmError::kIo;
    }
    out_randomness = randomness.value();
    return FvmError::kOk;
  }

  FvmError callbackFault(FvmMachineId machine_id,
                         BytesIn block1,
                         BytesIn block2,
                         BytesIn extra,
                         ConsensusFault &out_fault,
                         GasAmount &out_gas_used) {
    FVM_MACHINE();
    out_gas_used = {};
    auto fault{runtime::consensusFault(out_gas_used,
                                       machine->envx,
                                       machine->ts_branch,
                                       machine->epoch,
                                       machine->base_state,
                                       block1,
                                       block2,
                                       extra)};
    if (fault) {
      out_fault = std::move(fault.value());
    } else {
      out_fault = {};
    }
    return FvmError::kOk;
  }

  void initCallbacks() {
    [[maybe_unused]] static const auto once{[] {
      stub::callbacks = {
          callbackIpldGet,
          callbackIpldPut,
          callbackIpldPutMany,
          callbackRand,
          callbackFault,
      };
      return nullptr;
    }()};
  }
}  // namespace fc::vm::fvm
