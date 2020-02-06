/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/datastore_key.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>

namespace fc::storage::ipfs {

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

    return boost::filesystem::path(std::string(v)).normalize().string();
  }

  Key makeKeyFromString(std::string_view value) {
    return Key{formatKeyData(value)};
  }

  outcome::result<Key> makeRawKey(std::string_view value) {
    if (value.empty()) {
      return Key{"/"};
    }

    auto size = value.size();
    if (value[0] != '/' || (size > 1 && value[size - 1] == '/')) {
      return DatastoreKeyError::INVALID_DATASTORE_KEY;
    }

    return Key{std::string(value)};
  }

  inline bool operator<(const Key &lhs, const Key &rhs) {
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

  bool operator==(const Key &lhs, const Key &rhs) {
    return lhs.value == rhs.value;
  }
}  // namespace fc::storage::ipfs

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs, DatastoreKeyError, e) {
  using fc::storage::ipfs::DatastoreKeyError;

  switch (e) {
    case DatastoreKeyError::INVALID_DATASTORE_KEY:
      return "invalid data used to create datastore key";
  }

  return "unknown datastore key error";
}
