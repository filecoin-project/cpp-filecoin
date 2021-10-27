/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/program_options.hpp>

namespace fc::config {
  using boost::program_options::options_description;

  /**
   * Creates program option description for 'profile' and initialize parameters
   * according the profile.
   *
   * @return profile program option description
   */
  options_description configProfile();
}  // namespace fc::config
