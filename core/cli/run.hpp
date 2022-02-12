/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "cli/tree.hpp"

namespace fc::cli {
  inline bool isDash(const std::string &s) {
    return !s.empty() && s[0] == '-';
  }
  inline bool isDashDash(const std::string &s) {
    return s.size() == 2 && s[0] == '-' && s[1] == '-';
  }
  // note: returns first positional arg or end
  inline Argv::iterator hackBoost(const Opts &opts,
                                  Argv::iterator begin,
                                  Argv::iterator end) {
    po::parsed_options parsed{&opts};
    while (true) {
      if (begin == end) {
        break;
      }
      if (!isDash(*begin)) {
        break;
      }
      if (isDashDash(*begin)) {
        ++begin;
        break;
      }
      const auto it{std::find_if(begin + 1, end, isDash)};
      const auto options{
          po::command_line_parser{Argv{begin, it}}.options(opts).run().options};
      if (options.empty()) {
        break;
      }
      for (const auto &option : options) {
        parsed.options.emplace_back(option);
        if (option.string_key.empty()) {
          break;
        }
        begin += option.original_tokens.size();
      }
    }
    po::variables_map vm;
    po::store(parsed, vm);
    po::notify(vm);
    return begin;
  }

  inline RunResult run(std::string app, const Tree &_tree, Argv argv) {
    auto tree{&_tree};
    std::vector<std::string> cmds;
    cmds.emplace_back(std::move(app));
    ArgsMap argm;
    auto argv_it{argv.begin()};
    while (true) {
      auto args{tree->args()};
      auto option{args.opts.add_options()};
      bool help{};
      // note: help doesn't work with required options
      option("help,h", po::bool_switch(&help));
      argv_it = hackBoost(args.opts, argv_it, argv.end());
      argm._.emplace(args._);
      if (!help) {
        if (argv_it != argv.end()) {
          auto sub_it{tree->sub.find(*argv_it)};
          if (sub_it != tree->sub.end()) {
            ++argv_it;
            cmds.emplace_back(sub_it->first);
            tree = &sub_it->second;
            continue;
          }
        }
        if (tree->run) {
          try {
            return tree->run(argm, {argv_it, argv.end()});
          } catch (ShowHelp &) {
          }
        }
      }
      fmt::print("name:\n  {}\n", fmt::join(cmds, " "));
      fmt::print("options:\n{}", args.opts);
      if (!tree->sub.empty()) {
        fmt::print("subcommands:\n");
        for (const auto &sub : tree->sub) {
          fmt::print("  {}\n", sub.first);
        }
      }
      return;
    }
  }
  inline RunResult run(std::string app,
                       const Tree &tree,
                       int argc,
                       const char *argv[]) {
    return run(std::move(app), tree, {argv + 1, argv + argc});
  }
}  // namespace fc::cli
