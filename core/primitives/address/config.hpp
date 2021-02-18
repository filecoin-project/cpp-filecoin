/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/program_options.hpp>

#include "primitives/address/address.hpp"

namespace fc::primitives::address {
  inline void configCurrentNetwork(
      boost::program_options::options_description_easy_init &option) {
    option("use-mainnet-address-prefix",
           boost::program_options::bool_switch()->notifier([](auto mainnet) {
             currentNetwork = mainnet ? MAINNET : TESTNET;
           }));
  }
}  // namespace fc::primitives::address
