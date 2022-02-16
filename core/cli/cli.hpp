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

#define CLI_TRY(valueName, maybeResult)                                    \
  auto valueName##OUTCOME_TRY = maybeResult;                               \
  if (valueName##OUTCOME_TRY.has_error()) throw std::invalid_argument(""); \
  auto valueName = valueName##OUTCOME_TRY.value();

#define CLI_TRY_TEXT(valueName, maybeResult, textError) \
  auto valueName##OUTCOME_TRY = maybeResult;            \
  if (valueName##OUTCOME_TRY.has_error())               \
    throw std::invalid_argument(textError);             \
  auto valueName = valueName##OUTCOME_TRY.value();

#include "cli/try.hpp"

#define CLI_BOOL(NAME, DESCRIPTION)                               \
  struct {                                                        \
    bool _{};                                                     \
    void operator()(Opts &opts) {                                 \
      opts.add_options()(NAME, po::bool_switch(&_), DESCRIPTION); \
    }                                                             \
    operator bool() const {                                       \
      return _;                                                   \
    }                                                             \
  }
#define CLI_DEFAULT(NAME, DESCRIPTION, TYPE, INIT)          \
  struct {                                                  \
    TYPE _ INIT;                                            \
    void operator()(Opts &opts) {                           \
      opts.add_options()(NAME, po::value(&_), DESCRIPTION); \
    }                                                       \
    auto &operator*() const {                               \
      return _;                                             \
    }                                                       \
    auto *operator->() const {                              \
      return &_;                                            \
    }                                                       \
  }
#define CLI_OPTIONAL(NAME, DESCRIPTION, TYPE)                              \
  struct {                                                                 \
    boost::optional<TYPE> _;                                               \
    void operator()(Opts &opts) {                                          \
      opts.add_options()(NAME, po::value(&_), DESCRIPTION);                \
    }                                                                      \
    void operator=(TYPE &&rhs) {                                           \
      _ = std::move(rhs);                                                  \
    }                                                                      \
    operator bool() const {                                                \
      return _.operator bool();                                            \
    }                                                                      \
    auto &operator*() const {                                              \
      if (!_) {                                                            \
        throw ::fc::cli::CliError{"--{} argument is required but missing", \
                                  NAME};                                   \
      }                                                                    \
      return *_;                                                           \
    }                                                                      \
    auto *operator->() const {                                             \
      return &**this;                                                      \
    }                                                                      \
  }

#define CLI_OPTS() ::fc::cli::Opts opts()
#define CLI_RUN()                  \
  static ::fc::cli::RunResult run( \
      const ::fc::cli::ArgsMap &argm, Args &args, const ::fc::cli::Argv &argv)
#define CLI_NO_RUN() constexpr static nullptr_t run{nullptr};

namespace fc::cli {
  namespace po = boost::program_options;
  using Opts = po::options_description;

  using RunResult = void;
  struct ArgsMap {
    mutable std::map<std::type_index, std::shared_ptr<void>> _;
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

  const std::string &cliArgv(const Argv &argv,
                             size_t i,
                             const std::string_view &name) {
    if (i < argv.size()) {
      return argv[i];
    }
    throw CliError{"positional argument {} is required but missing", name};
  }
  template <typename T>
  T cliArgv(const std::string &arg, const std::string_view &name) {
    boost::any out;
    try {
      po::value<T>()->xparse(out, Argv{arg});
    } catch (po::validation_error &e) {
      e.set_option_name(std::string{name});
      throw;
    }
    return boost::any_cast<T>(out);
  }
  template <typename T>
  T cliArgv(const Argv &argv, size_t i, const std::string_view &name) {
    return cliArgv<T>(cliArgv(argv, i, name), name);
  }

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
