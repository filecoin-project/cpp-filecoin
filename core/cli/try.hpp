/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <boost/optional.hpp>

#include "common/outcome.hpp"
#include "common/outcome_fmt.hpp"

namespace fc::cli {
  struct CliError : std::runtime_error {
    template <typename... Args>
    CliError(const std::string_view &format, const Args &...args)
        : runtime_error{fmt::format(format, args...)} {}
  };

  template <typename R, typename... Args>
  auto cliTry(outcome::result<R> &&o,
              const std::string_view &format,
              const Args &...args) {
    if (o) {
      return std::move(o).value();
    }
    throw CliError{
        "{} (error_code: {:#})", fmt::format(format, args...), o.error()};
  }

  template <typename R>
  auto cliTry(outcome::result<R> &&o) {
    return cliTry(std::move(o), "outcome::result");
  }

  template <typename R, typename... Args>
  auto cliTry(boost::optional<R> &&o,
              const std::string_view &format,
              const Args &...args) {
    if (o) {
      return std::move(o).value();
    }
    throw CliError{format, args...};
  }

  template <typename R>
  auto cliTry(boost::optional<R> &&o) {
    return cliTry(std::move(o), "boost::optional");
  }
}  // namespace fc::cli
