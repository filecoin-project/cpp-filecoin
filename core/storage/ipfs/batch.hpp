/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPFS_BATCH_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPFS_BATCH_HPP

#include "storage/ipfs/datastore.hpp"

namespace fc::storage::ipfs {

  struct Batch : IpfsDatastore {
    Batch(std::shared_ptr<IpfsDatastore> base,
          std::shared_ptr<IpfsDatastore> diff)
        : base{std::move(base)}, diff{std::move(base)} {}

    ~Batch() override = default;

    outcome::result<bool> contains(const CID &key) const override {
      auto it = std::find(keys.begin(), keys.end(), key);
      if (it != keys.end()) {
        return true;
      }
      return base->contains(key);
    }

    outcome::result<void> set(const CID &key, Value value) override {
      auto it = std::find(keys.begin(), keys.end(), key);
      OUTCOME_TRY(base_has, base->contains(key));
      if (base_has) {
        OUTCOME_TRY(base_value, base->get(key));
        if (value == base_value) {
          if (it != keys.end()) {
            keys.erase(it);
          }
          return outcome::success();
        }
      }
      if (it == keys.end()) {
        keys.emplace_back(key);
      }
      OUTCOME_TRY(diff->set(key, value));
    }

    outcome::result<Value> get(const CID &key) const override {
      auto it = std::find(keys.begin(), keys.end(), key);
      return (it != keys.end() ? diff : base)->get(key);
    }

    virtual outcome::result<void> remove(const CID &key) override {
      auto it = std::find(keys.begin(), keys.end(), key);
      if (it == keys.end()) {
        return outcome::success();
      }

      keys.erase(it);
      return base->remove(key);
    }

    std::shared_ptr<IpfsDatastore> base, diff;
    std::vector<CID> keys;
  };
}  // namespace fc::storage::ipfs

#endif  // CPP_FILECOIN_CORE_STORAGE_IPFS_BATCH_HPP
