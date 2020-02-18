/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/datastore_key.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>

namespace fc::storage {

  std::string formatKeyData(std::string_view value) {
    if (value.empty()) {
      return "/";
    }

    std::string v;

    if (value[0] == '/') {
      v = value;
    } else {
      v = "/" + std::string(value);
    }

    auto res =
        boost::filesystem::path(std::string(v)).lexically_normal().string();

    if (res == "/.") {
      return "/";
    }

    auto len = res.length();

    // remove last /. if presents
    if (len > 2 && res.substr(len - 2, 2) == "/.") {
      return res.substr(0, len - 2);
    }

    return res;
  }

  DatastoreKey DatastoreKey::makeFromString(std::string_view value) noexcept {
    return DatastoreKey{formatKeyData(value)};
  }

  bool operator<(const DatastoreKey &lhs, const DatastoreKey &rhs) {
    std::vector<std::string> lhs_list;
    std::vector<std::string> rhs_list;
    boost::split(lhs_list, lhs.value, [](char c) { return c == '/'; });
    boost::split(rhs_list, rhs.value, [](char c) { return c == '/'; });
    for (size_t i = 0; i < lhs_list.size(); ++i) {
      if (rhs_list.size() < (i + 1)) {
        return false;
      }
      auto &li = lhs_list[i];
      auto &ri = rhs_list[i];
      if (li < ri) return true;
      if (li > ri) return false;
      // li == ri => continue
    }

    return lhs_list.size() < rhs_list.size();
  }

  bool operator==(const DatastoreKey &lhs, const DatastoreKey &rhs) {
    return lhs.value == rhs.value;
  }
}  // namespace fc::storage

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage, DatastoreKeyError, e) {
  using fc::storage::DatastoreKeyError;

  switch (e) {
    case DatastoreKeyError::INVALID_DATASTORE_KEY:
      return "invalid data used to create datastore key";
  }

  return "unknown datastore key error";
}
