/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/ipfs/datastore.hpp"
#include "storage/ipld/light_ipld.hpp"

namespace fc::storage::ipld {
  struct IpldIpld2 : LightIpld {
    IpldPtr ipld;

    IpldIpld2(IpldPtr ipld) : ipld{ipld} {}

    bool get(const Hash256 &key, Buffer *value) const override {
      auto r{ipld->get(asCborBlakeCid(key))};
      if (r == outcome::failure(storage::ipfs::IpfsDatastoreError::kNotFound)) {
        return false;
      }
      if (value) {
        *value = std::move(r.value());
      }
      return true;
    }

    void put(const Hash256 &key, BytesIn value) override {
      ipld->set(asCborBlakeCid(key), Buffer{value}).value();
    }
  };
}  // namespace fc::storage::ipld
