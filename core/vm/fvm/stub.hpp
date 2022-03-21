/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/randomness/randomness_types.hpp"
#include "vm/runtime/consensus_fault_types.hpp"

namespace fc::vm::fvm {
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;
  using primitives::GasAmount;

  using FvmMachineId = uint64_t;

  enum class FvmError : int32_t {
    kIpldHas = 1,
    kOk = 0,
    kInvalidHandle = -1,
    kNotFound = -2,
    kIo = -3,
    kInvalidArgument = -4,
  };
}  // namespace fc::vm::fvm

namespace fc::vm::fvm::stub {
  struct Callbacks {
    FvmError (*ipldGet)(FvmMachineId machine_id,
                        BytesIn key,
                        Bytes *out_value){};
    FvmError (*ipldPut)(FvmMachineId machine_id, BytesIn key, BytesIn value){};
    FvmError (*ipldPutMany)(FvmMachineId machine_id,
                            gsl::span<const int32_t> sizes,
                            BytesIn keys_values){};
    FvmError (*rand)(FvmMachineId machine_id,
                     bool beacon,
                     DomainSeparationTag tag,
                     ChainEpoch epoch,
                     BytesIn seed,
                     Randomness &out_randomness){};
    FvmError (*fault)(FvmMachineId machine_id,
                      BytesIn block1,
                      BytesIn block2,
                      BytesIn extra,
                      ConsensusFault &out_fault,
                      GasAmount &out_gas_used){};
  };
  extern Callbacks callbacks;
}  // namespace fc::vm::fvm::stub
