/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/multimap.hpp"

#include "adt/array.hpp"

namespace fc::adt {

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
    boost::optional<Array> array{boost::none};
    auto found = hamt_.getCbor<CID>(key);

    if (found) {
      array = Array(store_, found.value());
    } else {
      if (HamtError::NOT_FOUND == found.error()) {
        array = Array(store_);
      } else {
        return found.error();
      }
    }

    OUTCOME_TRY(array->append(value));
    OUTCOME_TRY(array_root, array->flush());
    return hamt_.setCbor<CID>(key, array_root);
  }

  outcome::result<void> Multimap::removeAll(const std::string &key) {
    return hamt_.remove(key);
  }

  outcome::result<void> Multimap::visit(const std::string &key,
                                        const Multimap::Visitor &visitor) {
    OUTCOME_TRY(array_root, hamt_.getCbor<CID>(key));
    return Array{store_, array_root}.visit(visitor);
  }

}  // namespace fc::adt
