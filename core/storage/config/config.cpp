/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/config/config.hpp"

#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>

using fc::storage::config::Config;

fc::outcome::result<void> Config::load(const std::string &filename) {
  if (!boost::filesystem::exists(filename))
    return ConfigError::kCannotOpenFile;
  try {
    boost::property_tree::read_json(filename, ptree_);
  } catch (const boost::property_tree::json_parser::json_parser_error &) {
    return ConfigError::kJSONParserError;
  }
  return fc::outcome::success();
}

fc::outcome::result<void> Config::save(const std::string &filename) {
  try {
    boost::property_tree::write_json(filename, ptree_);
  } catch (const boost::property_tree::file_parser_error &) {
    return ConfigError::kCannotOpenFile;
  }
  return fc::outcome::success();
}
