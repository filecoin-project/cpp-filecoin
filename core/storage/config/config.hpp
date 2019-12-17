/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CONFIG_HPP
#define CPP_FILECOIN_CORE_STORAGE_CONFIG_HPP

#include <boost/property_tree/ptree.hpp>
#include <string>

#include "common/outcome.hpp"
#include "storage/config/config_error.hpp"

namespace fc::storage::config {

  /** @brief Configuration key */
  using ConfigKey = std::string;

  /**
   * @brief Filecoin Node configuration
   */
  class Config {
   public:
    virtual ~Config() = default;

    /**
     * @brief Save config to file
     * @param filename - path to a file to store config
     * @return nothing or error occurred
     */
    outcome::result<void> save(const std::string &filename);

    /**
     * @brief Load config from file
     * @param filename - path to a file with config
     * @return nothing or error occurred
     */
    outcome::result<void> load(const std::string &filename);

    /**
     * Set config value
     * @tparam T - parameter of a configuration value
     * @param key - configuration key
     * @param value - configuration value
     * @return - nothing or error occurred
     */
    template <typename T>
    outcome::result<void> set(const ConfigKey &key, const T &value) {
      ptree_.put<T>(key, value);
      return outcome::success();
    }

    /**
     * Get config value by key
     * @tparam T - expected parameter of a configuration value
     * @param key - configuration key
     * @return value or error occurred
     */
    template <typename T>
    outcome::result<T> get(const ConfigKey &key) {
      try {
        return ptree_.get<T>(key);
      } catch (const boost::property_tree::ptree_bad_path &) {
        return ConfigError::BAD_PATH;
      }
    }

   private:
    boost::property_tree::ptree ptree_;
  };

}  // namespace fc::storage::config

#endif  // CPP_FILECOIN_CORE_STORAGE_CONFIG_HPP
