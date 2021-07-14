/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/ipfs/datastore.hpp"

namespace fc {
  struct IpldProxy : Ipld {
    IpldPtr ipld;

    explicit IpldProxy(IpldPtr ipld) : ipld{std::move(ipld)} {}

    outcome::result<bool> contains(const CID &key) const override {
      return ipld->contains(key);
    }
    outcome::result<void> set(const CID &key, Value value) override {
      return ipld->set(key, value);
    }
    outcome::result<Value> get(const CID &key) const override {
      return ipld->get(key);
    }
  };
}  // namespace fc
