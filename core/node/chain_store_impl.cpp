/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "chain_store_impl.hpp"

#include "common/logger.hpp"
#include "events.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::sync {

  using primitives::cid::getCidOfCbor;

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("chain_store");
      return logger.get();
    }
  }  // namespace

  ChainStoreImpl::ChainStoreImpl(
      std::shared_ptr<storage::ipfs::IpfsDatastore> ipld,
      std::shared_ptr<CachedInterpreter> interpreter,
      std::shared_ptr<WeightCalculator> weight_calculator,
      std::shared_ptr<BlockValidator> block_validator)
      : ipld_(std::move(ipld)),
        interpreter_(std::move(interpreter)),
        weight_calculator_(std::move(weight_calculator)),
        block_validator_(std::move(block_validator)) {
    assert(ipld_);
    assert(interpreter_);
    assert(weight_calculator_);
    assert(block_validator_);
  }

  outcome::result<void> ChainStoreImpl::start(
      std::shared_ptr<events::Events> events) {
    events_ = std::move(events);
    assert(events_);

    head_constructor_.start(events_);

    return outcome::success();
  }

  outcome::result<void> ChainStoreImpl::addBlock(const BlockHeader &block) {
    OUTCOME_TRY(cid, ipld_->setCbor(block));
    head_constructor_.blockFromApi(cid, block);
    return outcome::success();
  }

  TipsetCPtr ChainStoreImpl::heaviestTipset() const {
    assert(head_);
    return head_;
  }

  storage::blockchain::ChainStore::connection_t
  ChainStoreImpl::subscribeHeadChanges(
      const std::function<HeadChangeSignature> &subscriber) {
    assert(head_);
    subscriber(
        events::HeadChange{primitives::tipset::HeadChangeType::CURRENT, head_});
    return head_change_signal_.connect(subscriber);
  }

  primitives::BigInt ChainStoreImpl::getHeaviestWeight() const {
    return heaviest_weight_;
  }

  void ChainStoreImpl::newHeadChosen(TipsetCPtr tipset, BigInt weight) {
    using primitives::tipset::HeadChange;
    using primitives::tipset::HeadChangeType;

    // if (weight <= heaviest_weight_) {
    //   // should not get here
    //   return;
    // }

    // try {
    //   HeadChange event;

    //   bool first_head_apply = (head_ == nullptr);

    //   if (!first_head_apply) {
    //     assert(*head_ != *tipset);

    //     auto &ancestor = res.value();
    //     assert(ancestor);
    //     auto ancestor_height = ancestor->height();

    //     if (*ancestor != *head_) {
    //       assert(ancestor_height < head_->height());

    //       log()->info("fork detected, reverting from height {} to {}!",
    //                   head_->height(),
    //                   ancestor_height);

    //       event.type = HeadChangeType::REVERT;

    //       event.type = HeadChangeType::APPLY;

    //       // event.value = t;
    //       // head_change_signal_(event);
    //       // events_->signalHeadChange(event);
    //     }

    //     log()->info(
    //         "new current head: height={}, weight={}", tipset->height(),
    //         weight);

    //     event.type =
    //         first_head_apply ? HeadChangeType::CURRENT :
    //         HeadChangeType::APPLY;
    //     event.value = tipset;
    //     head_change_signal_(event);
    //     events_->signalHeadChange(event);
    //     head_ = std::move(tipset);
    //     heaviest_weight_ = std::move(weight);
    //   }
    //   catch (const std::system_error &e) {
    //     log()->error("cannot apply new head, {}", e.code().message());
    //   }

    events_->signalCurrentHead({.tipset = head_, .weight = heaviest_weight_});
  }

}  // namespace fc::sync

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::blockchain, ChainStoreError, e) {
  using E = fc::storage::blockchain::ChainStoreError;
  switch (e) {
    case E::kStoreNotInitialized:
      return "chain store error: not initialized";
    case E::kNoHeaviestTipset:
      return "chain store error: no heaviest tipset";
    case E::kNoTipsetAtHeight:
      return "chain store error: no tipset at required height";
    case E::kBlockRejected:
      return "chain store error: block rejected";
    case E::kIllegalState:
      return "chain store error: illegal state";
    default:
      break;
  }
  return "ChainStoreError: unknown error";
}
