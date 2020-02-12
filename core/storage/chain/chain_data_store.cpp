/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/chain_data_store.hpp"

#include "primitives/cid/cid_of_cbor.hpp"

namespace fc::storage::chain {
  using primitives::cid::getCidOfCbor;

  ChainDataStore::ChainDataStore(std::shared_ptr<ipfs::IpfsDatastore> store)
      : store_{std::move(store)} {
    BOOST_ASSERT_MSG(store_ != nullptr, "parameter store is nullptr");
  }

  outcome::result<std::string> ChainDataStore::get(
      const DatastoreKey &key) const {
    OUTCOME_TRY(cid, getCidOfCbor(key));
    OUTCOME_TRY(bytes, store_->get(cid));
    std::string str{bytes.begin(), bytes.end()};

    return str;
  }

  outcome::result<void> ChainDataStore::set(const DatastoreKey &key,
                                            std::string_view value) {
    OUTCOME_TRY(cid, getCidOfCbor(key));
    std::vector<uint8_t> bytes{value.begin(), value.end()};
    ipfs::IpfsDatastore::Value buffer{std::move(bytes)};
    return store_->set(cid, buffer);
  }

  outcome::result<bool> ChainDataStore::contains(
      const DatastoreKey &key) const {
    OUTCOME_TRY(cid, getCidOfCbor(key));
    return store_->contains(cid);
  }
}  // namespace fc::storage::chain
