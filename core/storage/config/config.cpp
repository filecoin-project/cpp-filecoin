/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/config/config.hpp"

using fc::storage::config::Config;

fc::outcome::result<void> Config::Load(
    const filestore::Path &filename) noexcept {
  boost::property_tree::read_xml(filename, ptree_);
  return fc::outcome::success();
}

fc::outcome::result<void> Config::Save(
    const filestore::Path &filename) noexcept {
  boost::property_tree::write_xml(filename, ptree_);
  return fc::outcome::success();
}
