/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/program_options/value_semantic.hpp>

#define CLI_VALIDATE(TYPE) \
  inline void validate(    \
      boost::any &out, const std::vector<std::string> &values, TYPE *, int)

namespace fc {
  template <typename F>
  void validateWith(boost::any &out,
                    const std::vector<std::string> &values,
                    const F &f) {
    namespace po = boost::program_options;
    po::check_first_occurrence(out);
    const auto &value{po::get_single_string(values)};
    try {
      out = f(value);
    } catch (...) {
      boost::throw_exception(po::invalid_option_value{value});
    }
  }
}  // namespace fc
