/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/fvm/stub.hpp"
#include "vm/runtime/env_context.hpp"
#include "vm/runtime/i_vm.hpp"

namespace fc::vm::fvm {
  using runtime::EnvironmentContext;

  struct FvmMachine : VirtualMachine {
    FvmMachineId machine_id;
    std::unique_ptr<void, void (*)(void *)> executor{nullptr, +[](void *) {}};
    EnvironmentContext envx;
    TsBranchPtr ts_branch;
    ChainEpoch epoch;
    CID base_state;

    static outcome::result<std::shared_ptr<FvmMachine>> make(
        EnvironmentContext envx,
        TsBranchPtr ts_branch,
        const TokenAmount &base_fee,
        const CID &state,
        ChainEpoch epoch);

    ~FvmMachine() override;
    outcome::result<ApplyRet> applyMessage(const UnsignedMessage &message,
                                           size_t size) override;
    outcome::result<MessageReceipt> applyImplicitMessage(
        const UnsignedMessage &message) override;
    outcome::result<CID> flush() override;
  };
}  // namespace fc::vm::fvm
