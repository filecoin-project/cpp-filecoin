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
    using namespace boost::program_options;

    // Make sure no previous assignment to 'v' was made.
    validators::check_first_occurrence(v);

    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    std::string const &value = validators::get_single_string(values);
    if (value == "mainnet" || value == "2k") {
      v = boost::any(Profile{value});
    } else {
      throw validation_error(validation_error::invalid_option_value);
    }
  }

  boost::program_options::options_description configProfile() {
    boost::program_options::options_description optionsDescription(
        "Profile options");
    optionsDescription.add_options()(
        "profile",
        boost::program_options::value<Profile>()
            ->default_value({"mainnet"})
            ->notifier([](const auto &profile) {
              if (profile == "mainnet") {
                // nothing because default values are mainnet
              } else if (profile == "2k") {
                setParams2K();
              }
            }),
        "Network parameters profile configuration that defines network "
        "update heights, network delays and etc. Supported profiles: \n"
        " * 'mainnet'\n"
        " * '2k'\n");

    return optionsDescription;
  }
}  // namespace fc::config
