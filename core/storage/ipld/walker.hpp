/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPLD_WALKER_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPLD_WALKER_HPP

#include "storage/ipfs/datastore.hpp"

namespace fc::storage::ipld::walker {
  using codec::cbor::CborDecodeStream;
  using Ipld = ipfs::IpfsDatastore;

  // TODO(turuslan): implement selectors
  struct Selector {
    common::Buffer raw;
  };
  CBOR_ENCODE(Selector, selector) {
    return s << s.wrap(selector.raw, 1);
  }
  CBOR_DECODE(Selector, selector) {
    selector.raw = common::Buffer{s.raw()};
    return s;
  }

  struct Walker {
    Walker(Ipld &store) : store{store} {}

    outcome::result<void> select(const CID &root, const Selector &selector);

    outcome::result<void> recursiveAll(const CID &cid);

    void recursiveAll(CborDecodeStream &s);

    Ipld &store;
    std::set<CID> visited;
    std::vector<CID> cids;
  };
}  // namespace fc::storage::ipld::walker

#endif  // CPP_FILECOIN_CORE_STORAGE_IPLD_WALKER_HPP
