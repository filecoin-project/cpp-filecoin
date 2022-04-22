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
  };

  struct UnionCli {
    std::string description;
    Tree::Sub sub = {};

    UnionCli(std::string description) : description(std::move(description)) {}

    UnionCli(std::map<std::string, Tree> sub) : sub(std::move(sub)) {}

    UnionCli(std::string description, const std::initializer_list<std::pair<std::string, Tree>>& sub) : description(std::move(description)) {
      for(const auto& order : sub) {
        this->sub[order.first] = order.second;
      }
    }

    UnionCli() = default;

    UnionCli(const std::initializer_list<std::pair<std::string, Tree>>& sub) {
      for(const auto& order : sub) {
        this->sub[order.first] = order.second;
      }
    }
  };

  template <typename Cmd>
  Tree tree(UnionCli dos) {
    Tree t;
    t.description = std::move(dos.description);
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
    t.sub = std::move(dos.sub);
    return t;
  }

}  // namespace fc::cli
