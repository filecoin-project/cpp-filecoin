/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/ipfs/datastore.hpp"

namespace fc {
  // TODO(turuslan): refactor Ipld to CbIpld
  struct IpldProxy : Ipld {
    IpldPtr ipld;

    explicit IpldProxy(IpldPtr ipld) : ipld{std::move(ipld)} {}

    outcome::result<bool> contains(const CID &key) const override {
      return ipld->contains(key);
    }
    outcome::result<void> set(const CID &key, BytesCow &&value) override {
      return ipld->set(key, std::move(value));
    }
    outcome::result<Value> get(const CID &key) const override {
      return ipld->get(key);
    }
  };

  // TODO(turuslan): refactor Ipld to CbIpld
  inline IpldPtr withVersion(IpldPtr ipld, ChainEpoch height) {
    const auto version{actorVersion(height)};
    if (ipld->actor_version != version) {
      ipld = std::make_shared<IpldProxy>(ipld);
      ipld->actor_version = version;
    }
    return ipld;
  }
}  // namespace fc
