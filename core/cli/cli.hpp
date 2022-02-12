/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/optional.hpp>
#include <boost/program_options/options_description.hpp>
#include <map>
#include <memory>
#include <typeindex>

#define CLI_TRY(valueName, maybeResult)                                \
  auto valueName##OUTCOME_TRY = maybeResult;                               \
  if (valueName##OUTCOME_TRY.has_error()) throw std::invalid_argument(""); \
  auto valueName = valueName##OUTCOME_TRY.value();

#define CLI_TRY_TEXT(valueName, maybeResult, textError)                                \
  auto valueName##OUTCOME_TRY = maybeResult;                               \
  if (valueName##OUTCOME_TRY.has_error()) throw std::invalid_argument(textError); \
  auto valueName = valueName##OUTCOME_TRY.value();

#define CLI_OPTS() ::fc::cli::Opts opts()
#define CLI_RUN()                                                 \
  static ::fc::cli::RunResult run(const ::fc::cli::ArgsMap &argm, \
                                  const Args &args,               \
                                  const ::fc::cli::Argv &argv)
#define CLI_NO_RUN() constexpr static nullptr_t run{nullptr};

namespace fc::cli {
  namespace po = boost::program_options;
  using Opts = po::options_description;

  using RunResult = void;
  struct ArgsMap {
    std::map<std::type_index, std::shared_ptr<void>> _;
    template <typename Args>
    void add(Args &&v) {
      _.emplace(typeid(Args), std::make_shared<Args>(std::forward<Args>(v)));
    }
    template <typename Cmd>
    const typename Cmd::Args &of() const {
      return *reinterpret_cast<const typename Cmd::Args *>(
          _.at(typeid(typename Cmd::Args)).get());
    }
  };
  // note: Args is defined inside command
  using Argv = std::vector<std::string>;

  struct Empty {
    struct Args {
      CLI_OPTS() {
        return {};
      }
    };
    CLI_NO_RUN();
  };
  using Group = Empty;

  struct ShowHelp {};
}  // namespace fc::cli
