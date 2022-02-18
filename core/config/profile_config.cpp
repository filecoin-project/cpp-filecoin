/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config/profile_config.hpp"

#include "const.hpp"

namespace fc::config {
  /**
   * Class for profile name validation with boost::program_options
   */
  class Profile : public std::string {};

  /**
   * Checks that profile name is expected one.
   */
  void validate(boost::any &v,
                std::vector<std::string> const &values,
                Profile *,
                int) {
    using boost::program_options::validation_error;
    using boost::program_options::validators::check_first_occurrence;
    using boost::program_options::validators::get_single_string;

    // Make sure no previous assignment to 'v' was made.
    check_first_occurrence(v);

    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    std::string const &value = get_single_string(values);
    if (value == "mainnet" || value == "2k" || value == "no-upgrades"
        || value == "interopnet" || value == "butterflynet") {
      v = boost::any(Profile{value});
    } else {
      throw validation_error(validation_error::invalid_option_value);
    }
  }

  options_description configProfile() {
    options_description optionsDescription("Profile options");
    optionsDescription.add_options()(
        "profile",
        boost::program_options::value<Profile>()
            ->default_value({"mainnet"})
            ->notifier([](const auto &profile) {
              if (profile == "mainnet") {
                // nothing because default values are mainnet
              } else if (profile == "2k") {
                setParams2K();
              } else if (profile == "no-upgrades") {
                setParamsNoUpgrades();
              } else if (profile == "interopnet") {
                setParamsInteropnet();
              } else if (profile == "butterflynet") {
                setParamsButterfly();
              }
            }),
        "Network parameters profile configuration that defines network "
        "update heights, network delays and etc. Supported profiles: \n"
        " * 'mainnet'\n"
        " * '2k'\n");
    optionsDescription.add_options()(
        "fake-winning-post",
        boost::program_options::bool_switch()->notifier(
            [](bool fake) { kFakeWinningPost = fake; }));

    return optionsDescription;
  }
}  // namespace fc::config
