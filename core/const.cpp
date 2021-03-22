/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "const.hpp"
#include "common/error_text.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/types/payment_channel/policy.hpp"
#include "vm/actor/builtin/types/storage_power/policy.hpp"
#include "vm/actor/builtin/types/verified_registry/policy.hpp"

namespace fc {
  using primitives::sector::RegisteredSealProof;

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
    std::string const &s = validators::get_single_string(values);
    if (s == "mainnet" || s == "2k") {
      v = boost::any(s);
    } else {
      throw validation_error(validation_error::invalid_option_value);
    }
  }

  void setParams2K() {
    kEpochDurationSeconds = 4;
    kEpochsInHour = kSecondsInHour / kEpochDurationSeconds;
    kEpochsInDay = 24 * kEpochsInHour;
    kEpochsInYear = 365 * kEpochsInDay;

    kPropagationDelaySecs = 1;

    // Network versions
    kUpgradeBreezeHeight = -1;
    kUpgradeSmokeHeight = -1;
    kUpgradeIgnitionHeight = -2;
    kUpgradeRefuelHeight = -3;
    kUpgradeTapeHeight = -4;
    kUpgradeActorsV2Height = 10;
    kUpgradeLiftoffHeight = -5;
    kUpgradeKumquatHeight = 15;
    kUpgradeCalicoHeight = 20;
    kUpgradePersianHeight = 25;
    kUpgradeOrangeHeight = 27;
    kUpgradeClausHeight = 30;
    kUpgradeActorsV3Height = 35;

    // Update policies
    std::set<RegisteredSealProof> supportedProofs;
    supportedProofs.insert(RegisteredSealProof::kStackedDrg2KiBV1);
    vm::actor::builtin::types::miner::setPolicy(kEpochDurationSeconds,
                                                supportedProofs);
    vm::actor::builtin::types::storage_power::setPolicy(2048);
    vm::actor::builtin::types::verified_registry::setPolicy(256);
    vm::actor::builtin::types::payment_channel::setPolicy(kEpochsInHour);
    vm::actor::builtin::types::market::setPolicy(kEpochsInDay);
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
}  // namespace fc
