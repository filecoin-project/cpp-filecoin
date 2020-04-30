/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipld/walker.hpp"

namespace fc::storage::ipld::walker {
  outcome::result<void> Walker::select(const CID &root,
                                       const Selector &selector) {
    // TODO(turuslan): implement selectors
    return recursiveAll(root);
  }

  outcome::result<void> Walker::recursiveAll(const CID &cid) {
    if (cids.insert(cid).second) {
      OUTCOME_TRY(bytes, store.get(cid));
      // TODO(turuslan): what about other types?
      // TODO(turuslan): DAG_PB, if required
      if (cid.content_type == libp2p::multi::MulticodecType::DAG_CBOR) {
        try {
          CborDecodeStream s{bytes};
          recursiveAll(s);
        } catch (std::system_error &e) {
          return outcome::failure(e.code());
        }
      }
    }
    return outcome::success();
  }

  void Walker::recursiveAll(CborDecodeStream &s) {
    if (s.isCid()) {
      CID cid;
      s >> cid;
      auto result = recursiveAll(cid);
      if (!result) {
        outcome::raise(result.error());
      }
    } else if (s.isList()) {
      auto n = s.listLength();
      for (auto l = s.list(); n != 0; --n) {
        recursiveAll(l);
      }
    } else if (s.isMap()) {
      for (auto &p : s.map()) {
        recursiveAll(p.second);
      }
    } else {
      s.next();
    }
  }
}  // namespace fc::storage::ipld::walker
