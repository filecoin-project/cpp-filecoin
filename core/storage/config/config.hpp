/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CONFIG_STORAGE_HPP
#define CPP_FILECOIN_CORE_STORAGE_CONFIG_STORAGE_HPP

#include <boost/any.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <string>

#include "common/outcome.hpp"
#include "storage/filestore/path.hpp"

namespace fc::storage::config {

  using ConfigKey = std::string;

  /**
   * @brief Filecoin Node configuration
   */
  class Config {
   public:
    virtual ~Config() = default;

    virtual outcome::result<void> Save(
        const filestore::Path &filename) noexcept;

    virtual outcome::result<void> Load(
        const filestore::Path &filename) noexcept;

    /**
     * Set config value
     * @tparam T
     * @param key
     * @param value
     * @return
     */
    template <typename T>
    outcome::result<void> Set(const ConfigKey &key,
                              const T &value) noexcept {
      ptree_.put<T>(key, value);
      return outcome::success();
    }

    /**
     * Get config value by path
     * @tparam T
     * @param key
     * @return
     */
    template <typename T>
    outcome::result<T> Get(const ConfigKey &key) noexcept {
      return ptree_.get<T>(key);
    }

   private:
    boost::property_tree::ptree ptree_;
  };

}  // namespace fc::storage::config

#endif  // CPP_FILECOIN_CORE_STORAGE_CONFIG_STORAGE_HPP
