/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/cli.hpp"

namespace fc::cli {
  struct Tree {
    using Sub = std::map<std::string, Tree>;
    struct Args {
      std::pair<std::type_index, std::shared_ptr<void>> _;
      Opts opts;
    };
    std::function<Args()> args;
    std::function<RunResult(ArgsMap &argm, Argv &&argv)> run;
    Sub sub;
  };
  template <typename Cmd>
  Tree tree(Tree::Sub sub = {}) {
    Tree t;
    t.args = [] {
      const auto ptr{std::make_shared<typename Cmd::Args>()};
      return Tree::Args{{typeid(typename Cmd::Args), ptr}, ptr->opts()};
    };
    constexpr auto run{
        !std::is_same_v<decltype(Cmd::run), const std::nullptr_t>};
    if constexpr (run) {
      t.run = [](ArgsMap &argm, Argv &&argv) {
        if constexpr (run) {
          return Cmd::run(argm, argm.of<Cmd>(), std::move(argv));
        }
      };
    }
    t.sub = std::move(sub);
    return t;
  }
}  // namespace fc::cli
