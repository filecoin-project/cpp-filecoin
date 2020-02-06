/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/chain_store.hpp"

#include "common/outcome.hpp"
#include "storage/ipfs/datastore_key.hpp"
#include "primitives/cid/cid_of_cbor.hpp"

namespace fc::storage::chain {

  namespace {
    static ipfs::Key chainHeadKey = ipfs::makeKeyFromString("head");
  }

  outcome::result<ChainStore> ChainStore::create(
      std::shared_ptr<ipfs::BlockService> bs,
      std::shared_ptr<ipfs::IpfsDatastore> ds,
      std::shared_ptr<chain::block_validator::BlockValidator> block_validator) {
    ChainStore cs{};
    cs.block_service_ = std::move(bs);
    cs.data_store_ = std::move(ds);
    cs.block_validator_ = std::move(block_validator);

    // TODO (yuraz) add notifications

    return cs;
  }

//  outcome::result<void> ChainStore::writeHead(
//      const primitives::tipset::Tipset &tipset) {}

  outcome::result<void> ChainStore::load() {
    auto &&key = primitives::cid::getCidOfCbor(chainHeadKey.value);
    OUTCOME_TRY(res, data_store_->get(chainHeadKey));
  }
}  // namespace fc::storage::chain
