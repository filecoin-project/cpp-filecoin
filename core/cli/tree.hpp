/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/cli.hpp"
#include <string>

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
    std::string description;
    std::vector<std::string> argusage;
  };

  template <typename Cmd>
  Tree tree(Tree::Sub dos = {}) {
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
    t.sub = std::move(dos);
    return t;
  }
  template<class Cmd>
  auto treeDesc(std::string desc, std::vector<std::string> argusage = {}) {
    return [=](Tree::Sub sub = {}) {
      auto t = tree<Cmd>(sub);
      t.description = desc;
      t.argusage = argusage;
      return t;
    }; }

}  // namespace fc::cli
