/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/program_options.hpp>

#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"

namespace fc::primitives::address {
  inline void configCurrentNetwork(
      boost::program_options::options_description_easy_init &option) {
    option("use-mainnet-address-prefix",
           boost::program_options::bool_switch()->notifier([](auto mainnet) {
             currentNetwork = mainnet ? MAINNET : TESTNET;
           }));
  }

  inline void validate(boost::any &out,
                       const std::vector<std::string> &values,
                       Address *,
                       long) {
    using namespace boost::program_options;
    check_first_occurrence(out);
    auto &value{get_single_string(values)};
    if (auto address{decodeFromString(value)}) {
      out = address.value();
      return;
    }
    boost::throw_exception(invalid_option_value{value});
  }
}  // namespace fc::primitives::address
