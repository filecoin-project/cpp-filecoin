/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/fvm/fvm.hpp"
#include "vm/runtime/env.hpp"

namespace fc::vm {
  using runtime::EnvironmentContext;
  using runtime::IpldBuffered;

  inline outcome::result<std::shared_ptr<VirtualMachine>> makeVm(
      const std::shared_ptr<IpldBuffered> &ipld,
      EnvironmentContext envx,
      const TsBranchPtr &ts_branch,
      const TokenAmount &base_fee,
      const CID &state,
      ChainEpoch epoch) {
    envx.ipld = ipld;
    const auto network{version::getNetworkVersion(epoch)};
    std::shared_ptr<VirtualMachine> vm;
    static const bool fvm_flag{[] {
      const auto s{getenv("FUHON_USE_FVM_EXPERIMENTAL")};
      return s && std::string_view{s} == "1";
    }()};
    const auto fvm{network > NetworkVersion::kVersion15 || fvm_flag};
    if (fvm) {
      OUTCOME_TRYA(
          vm, fvm::FvmMachine::make(envx, ts_branch, base_fee, state, epoch));
    } else {
      OUTCOME_TRYA(vm,
                   runtime::Env::make(envx, ts_branch, base_fee, state, epoch));
    }
    return vm;
  }
}  // namespace fc::vm
