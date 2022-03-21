/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/fvm/stub.hpp"

namespace fc::vm::fvm::stub {
  decltype(callbacks) callbacks;

  extern "C" FvmError cgo_blockstore_get(FvmMachineId machine_id,
                                         const uint8_t *key,
                                         int32_t key_size,
                                         uint8_t **out_value,
                                         int32_t *out_value_size) {
    if (!callbacks.ipldGet) {
      return FvmError::kInvalidHandle;
    }
    if (!out_value) {
      return FvmError::kInvalidArgument;
    }
    if (!out_value_size) {
      return FvmError::kInvalidArgument;
    }
    Bytes value;
    const auto res{callbacks.ipldGet(machine_id, {key, key_size}, &value)};
    if (res == FvmError::kOk) {
      *out_value = gsl::narrow<uint8_t *>(malloc(value.size()));
      memcpy(*out_value, value.data(), value.size());
      *out_value_size = value.size();
    }
    return res;
  }

  extern "C" FvmError cgo_blockstore_put(FvmMachineId machine_id,
                                         const uint8_t *key,
                                         int32_t key_size,
                                         const uint8_t *value,
                                         int32_t value_size) {
    if (!callbacks.ipldPut) {
      return FvmError::kInvalidHandle;
    }
    return callbacks.ipldPut(machine_id, {key, key_size}, {value, value_size});
  }

  extern "C" FvmError cgo_blockstore_put_many(FvmMachineId machine_id,
                                              const int32_t *sizes,
                                              int32_t count,
                                              const uint8_t *keys_values) {
    if (!callbacks.ipldPutMany) {
      return FvmError::kInvalidHandle;
    }
    constexpr ptrdiff_t kMax{size_t{1} << 30};
    if (count > kMax) {
      return FvmError::kInvalidArgument;
    }
    ptrdiff_t sum{0};
    const auto sizes2{gsl::make_span(sizes, count)};
    for (const auto size : sizes2) {
      if (size > kMax) {
        return FvmError::kInvalidArgument;
      }
      sum += size;
    }
    return callbacks.ipldPutMany(machine_id, sizes2, {keys_values, sum});
  }

  extern "C" FvmError cgo_blockstore_has(FvmMachineId machine_id,
                                         const uint8_t *key,
                                         int32_t key_size) {
    if (!callbacks.ipldGet) {
      return FvmError::kInvalidHandle;
    }
    return callbacks.ipldGet(machine_id, {key, key_size}, nullptr);
  }

  extern "C" FvmError cgo_extern_get_chain_randomness(
      FvmMachineId machine_id,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      const uint8_t *seed,
      int32_t seed_size,
      Randomness *out_randomness) {
    if (!callbacks.rand) {
      return FvmError::kInvalidHandle;
    }
    if (!out_randomness) {
      return FvmError::kInvalidArgument;
    }
    return callbacks.rand(
        machine_id, false, tag, epoch, {seed, seed_size}, *out_randomness);
  }

  extern "C" FvmError cgo_extern_get_beacon_randomness(
      FvmMachineId machine_id,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      const uint8_t *seed,
      int32_t seed_size,
      Randomness *out_randomness) {
    if (!callbacks.rand) {
      return FvmError::kInvalidHandle;
    }
    if (!out_randomness) {
      return FvmError::kInvalidArgument;
    }
    return callbacks.rand(
        machine_id, true, tag, epoch, {seed, seed_size}, *out_randomness);
  }

  extern "C" FvmError cgo_extern_verify_consensus_fault(
      FvmMachineId machine_id,
      const uint8_t *block1,
      int32_t block1_size,
      const uint8_t *block2,
      int32_t block2_size,
      const uint8_t *extra,
      int32_t extra_size,
      ActorId *out_miner_id,
      ChainEpoch *out_epoch,
      ConsensusFaultType *out_fault,
      GasAmount *out_gas_used) {
    if (!callbacks.fault) {
      return FvmError::kInvalidHandle;
    }
    if (!out_miner_id) {
      return FvmError::kInvalidArgument;
    }
    if (!out_epoch) {
      return FvmError::kInvalidArgument;
    }
    if (!out_fault) {
      return FvmError::kInvalidArgument;
    }
    if (!out_gas_used) {
      return FvmError::kInvalidArgument;
    }
    ConsensusFault fault;
    const auto res{callbacks.fault(machine_id,
                                   {block1, block1_size},
                                   {block2, block2_size},
                                   {extra, extra_size},
                                   fault,
                                   *out_gas_used)};
    if (res == FvmError::kOk) {
      *out_miner_id = fault.target;
      *out_epoch = fault.epoch;
      *out_fault = fault.type;
    }
    return res;
  }
}  // namespace fc::vm::fvm::stub
