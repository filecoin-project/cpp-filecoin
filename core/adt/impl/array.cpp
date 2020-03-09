/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/adt/array.hpp"

namespace fc::adt {

  Array::Array(const std::shared_ptr<storage::ipfs::IpfsDatastore> &store)
      : store_(store), amt_(store) {}

  Array::Array(const std::shared_ptr<storage::ipfs::IpfsDatastore> &store,
               const CID &root)
      : store_(store), amt_(store, root) {}

  outcome::result<CID> Array::flush() {
    return amt_.flush();
  }

  outcome::result<void> Array::append(gsl::span<const uint8_t> value) {
    OUTCOME_TRY(count, amt_.count());
    return amt_.set(count, value);
  }

  outcome::result<void> Array::visit(const Array::IndexedVisitor &visitor) {
    return amt_.visit(visitor);
  }

  outcome::result<void> Array::visit(const Array::Visitor &visitor) {
    return amt_.visit(
        [&visitor](auto, const Value &value) { return visitor(value); });
  }

}  // namespace fc::adt
