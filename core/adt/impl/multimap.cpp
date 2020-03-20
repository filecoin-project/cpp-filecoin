/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/multimap.hpp"

#include "storage/amt/amt.hpp"

namespace fc::adt {
  using storage::amt::Amt;

  Multimap::Multimap(const std::shared_ptr<storage::ipfs::IpfsDatastore> &store)
      : store_(store), hamt_(store) {}

  Multimap::Multimap(const std::shared_ptr<storage::ipfs::IpfsDatastore> &store,
                     const CID &root)
      : store_(store), hamt_(store, root) {}

  outcome::result<CID> Multimap::flush() {
    return hamt_.flush();
  }

  outcome::result<void> Multimap::add(const std::string &key,
                                      gsl::span<const uint8_t> value) {
    using storage::hamt::HamtError;
    Amt amt{store_};
    OUTCOME_TRY(found, hamt_.contains(key));
    if (found) {
      OUTCOME_TRY(root, hamt_.getCbor<CID>(key));
      amt = Amt{store_, root};
    }
    OUTCOME_TRY(count, amt.count());
    OUTCOME_TRY(amt.set(count, value));
    OUTCOME_TRY(amt.flush());
    return hamt_.setCbor(key, amt.cid());
  }

  outcome::result<void> Multimap::removeAll(const std::string &key) {
    return hamt_.remove(key);
  }

  outcome::result<void> Multimap::visit(const std::string &key,
                                        const Multimap::Visitor &visitor) {
    OUTCOME_TRY(array_root, hamt_.getCbor<CID>(key));
    return Amt{store_, array_root}.visit(
        [&](auto, auto &value) { return visitor(value); });
  }

}  // namespace fc::adt
