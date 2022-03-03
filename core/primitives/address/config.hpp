/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/program_options.hpp>

#include "cli/validate/address.hpp"

namespace fc::primitives::address {
  inline void configCurrentNetwork(
      boost::program_options::options_description_easy_init &option) {
    option("use-mainnet-address-prefix",
           boost::program_options::bool_switch()->notifier([](auto mainnet) {
             currentNetwork = mainnet ? MAINNET : TESTNET;
           }));
  }
}  // namespace fc::primitives::address
