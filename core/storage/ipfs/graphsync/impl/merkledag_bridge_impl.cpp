/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "merkledag_bridge_impl.hpp"

#include <cassert>

#include <libp2p/multi/content_identifier_codec.hpp>
#include "storage/ipfs/merkledag/merkledag_service.hpp"

using libp2p::multi::ContentIdentifierCodec;

namespace fc::storage::ipfs::graphsync {

  std::shared_ptr<MerkleDagBridge> MerkleDagBridge::create(
      std::shared_ptr<merkledag::MerkleDagService> service) {
    return std::make_shared<MerkleDagBridgeImpl>(std::move(service));
  }

  MerkleDagBridgeImpl::MerkleDagBridgeImpl(
      std::shared_ptr<merkledag::MerkleDagService> service)
      : service_(std::move(service)) {
    assert(service_);
  }

  outcome::result<size_t> MerkleDagBridgeImpl::select(
      const CID& root_cid,
      gsl::span<const uint8_t> selector,
      std::function<bool(const CID &, const common::Buffer &)> handler) const {
    auto internal_handler =
        [&handler](std::shared_ptr<const merkledag::Node> node) -> bool {
      return handler(node->getCID(), node->getRawBytes());
    };

    if (selector.empty()) {
      OUTCOME_TRY(node, service_->getNode(root_cid));
      internal_handler(node);
      return 1;
    }

    // TODO(artem): change MerkleDAG service to accept CID instead of bytes
    OUTCOME_TRY(cid_encoded, ContentIdentifierCodec::encode(root_cid));
    return service_->select(cid_encoded, selector, internal_handler);
  }

}  // namespace fc::storage::ipfs::graphsync
