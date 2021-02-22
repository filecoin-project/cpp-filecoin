/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "fwd.hpp"

namespace fc::vm::runtime {
  using actor::Invoker;
  using interpreter::InterpreterCache;

  struct Env0 {
    IpldPtr ipld;
    std::shared_ptr<Invoker> invoker;
    std::shared_ptr<RuntimeRandomness> randomness;
    TsLoadPtr ts_load{};
    std::shared_ptr<InterpreterCache> interpreter_cache{};
    std::shared_ptr<Circulating> circulating{};
  };
}  // namespace fc::vm::runtime
