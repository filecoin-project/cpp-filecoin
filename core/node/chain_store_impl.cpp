/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "chain_store_impl.hpp"

#include "chain_db.hpp"
#include "common/logger.hpp"
#include "events.hpp"
#include "primitives/cid/cid_of_cbor.hpp"

namespace fc::sync {

  using primitives::cid::getCidOfCbor;

  ChainStoreImpl::ChainStoreImpl(
      std::shared_ptr<ChainDb> chain_db,
      std::shared_ptr<storage::ipfs::IpfsDatastore> ipld,
      std::shared_ptr<WeightCalculator> weight_calculator)
      : chain_db_{std::move(chain_db)},
        ipld_{std::move(ipld)},
        weight_calculator_{std::move(weight_calculator)} {
    assert(chain_db_);
    assert(ipld_);
    assert(weight_calculator_);
  }

  void ChainStoreImpl::start(std::shared_ptr<events::Events> events,
                             std::string network_name) {
    events_ = std::move(events);
    network_name_ = std::move(network_name);
    assert(events_);

    head_constructor_.start(events_);

    // get heads and interpret
  }

  outcome::result<void> ChainStoreImpl::addBlock(const BlockHeader &block) {
    OUTCOME_TRY(cid, ipld_->setCbor(block));
    head_constructor_.blockFromApi(cid, block);
    return outcome::success();
  }

  outcome::result<TipsetCPtr> ChainStoreImpl::loadTipset(
      const TipsetHash &hash) {
    return chain_db_->getTipsetByHash(hash);
  }

  outcome::result<TipsetCPtr> ChainStoreImpl::loadTipset(const TipsetKey &key) {
    return chain_db_->getTipsetByKey(key);
  }

  outcome::result<TipsetCPtr> ChainStoreImpl::loadTipsetByHeight(
      uint64_t height) {
    return chain_db_->getTipsetByHeight(height);
  }

  TipsetCPtr ChainStoreImpl::heaviestTipset() const {
    assert(head_);
    return head_;
  }

  storage::blockchain::ChainStore::connection_t
  ChainStoreImpl::subscribeHeadChanges(
      const std::function<HeadChangeSignature> &subscriber) {
    assert(events_);
    if (head_) {
      subscriber(events::HeadChange{primitives::tipset::HeadChangeType::CURRENT,
                                    head_});
    }
    return events_->subscribeHeadChange(subscriber);
  }

  const std::string& ChainStoreImpl::getNetworkName() const {
    return network_name_;
  }

  const CID& ChainStoreImpl::genesisCID() const {
    return chain_db_->genesisCID();
  }

  primitives::BigInt ChainStoreImpl::getHeaviestWeight() const {
    return heaviest_weight_;
  }

//  outcome::result<void> ChainStoreImpl::notifyHeadChange(const Tipset &current,
//                                                         const Tipset &target) {
//    OUTCOME_TRY(path, findChainPath(current, target));
//
//    for (auto &revert_item : path.revert_chain) {
//      head_change_signal_(
//          HeadChange{.type = HeadChangeType::REVERT, .value = revert_item});
//    }
//
//    for (auto &apply_item : path.apply_chain) {
//      head_change_signal_(
//          HeadChange{.type = HeadChangeType::APPLY, .value = apply_item});
//    }
//
//    return outcome::success();
//  }


}  // namespace fc::sync

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::blockchain, ChainStoreError, e) {
  using fc::storage::blockchain::ChainStoreError;
  //  switch (e) {
  //    case ChainStoreError::kNoPath:
  //      return "no path";
  //  }
  return "ChainStoreError: unknown error";
}
