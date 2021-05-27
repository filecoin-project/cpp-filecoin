/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cassert>

#include "cbor_blake/ipld.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc {
  struct CbAsAnyIpld : Ipld, std::enable_shared_from_this<CbAsAnyIpld> {
    CbIpldPtr ipld;

    CbAsAnyIpld(CbIpldPtr ipld) : ipld{std::move(ipld)} {}

    outcome::result<bool> contains(const CID &key) const override {
      if (auto cid{asBlake(key)}) {
        return ipld->has(*cid);
      }
      return false;
    }
    outcome::result<void> set(const CID &key, Value value) override {
      auto cid{asBlake(key)};
      assert(cid);
      ipld->put(*cid, value);
      return outcome::success();
    }
    outcome::result<Value> get(const CID &key) const override {
      if (auto cid{asBlake(key)}) {
        Buffer value;
        if (ipld->get(*cid, value)) {
          return std::move(value);
        }
      }
      return storage::ipfs::IpfsDatastoreError::kNotFound;
    }
    IpldPtr shared() override {
      return shared_from_this();
    }
  };

  struct AnyAsCbIpld : CbIpld {
    using CbIpld::get, CbIpld::put;

    IpldPtr ipld;

    AnyAsCbIpld(IpldPtr ipld) : ipld{std::move(ipld)} {}

    static bool get(const IpldPtr &ipld, const CbCid &key, Buffer *value) {
      auto cid{asCborBlakeCid(key)};
      if (value) {
        if (auto r{ipld->get(cid)}) {
          *value = std::move(r.value());
          return true;
        } else if (r.error() != storage::ipfs::IpfsDatastoreError::kNotFound) {
          r.value();  // throws
        }
        return false;
      }
      return ipld->contains(cid).value();
    }
    bool get(const CbCid &key, Buffer *value) const override {
      return get(ipld, key, value);
    }
    void put(const CbCid &key, BytesIn value) override {
      ipld->set(asCborBlakeCid(key), Buffer{value}).value();
    }
  };
}  // namespace fc
