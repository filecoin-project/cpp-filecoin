/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/merkledag/impl/node_impl.hpp"

#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/multi/multibase_codec/codecs/base58.hpp>
#include <libp2p/multi/multihash.hpp>
#include "merkledag.pb.h"
#include "storage/ipfs/merkledag/impl/pb_node_decoder.hpp"

using libp2p::common::Hash256;
using libp2p::multi::ContentIdentifierCodec;
using libp2p::multi::HashType;
using libp2p::multi::MulticodecType;
using libp2p::multi::Multihash;
using merkledag::pb::PBLink;
using merkledag::pb::PBNode;
using Version = libp2p::multi::ContentIdentifier::Version;

namespace fc::storage::ipfs::merkledag {

  size_t NodeImpl::size() const {
    return getCachePB().size() + child_nodes_size_;
  }

  void NodeImpl::assign(common::Buffer input) {
    content_ = std::move(input);
    pb_cache_ = boost::none;
  }

  const common::Buffer &NodeImpl::content() const {
    return content_;
  }

  outcome::result<void> NodeImpl::addChild(const std::string &name,
                                           std::shared_ptr<const Node> node) {
    LinkImpl link{node->getCID(), name, node->size()};
    links_.emplace(name, std::move(link));
    pb_cache_ = boost::none;
    child_nodes_size_ += node->size();
    return outcome::success();
  }

  outcome::result<std::reference_wrapper<const Link>> NodeImpl::getLink(
      const std::string &name) const {
    if (auto index = links_.find(name); index != links_.end()) {
      return index->second;
    }
    return NodeError::LINK_NOT_FOUND;
  }

  void NodeImpl::removeLink(const std::string &link_name) {
    if (auto index = links_.find(link_name); index != links_.end()) {
      child_nodes_size_ -= index->second.getSize();
      links_.erase(index);
    }
  }

  const CID &NodeImpl::getCID() const {
    if (!cid_) {
      Hash256 digest = libp2p::crypto::sha256(getCachePB().toVector());
      auto multi_hash = Multihash::create(HashType::sha256, digest);
      CID id{
          Version::V0, MulticodecType::DAG_PB, std::move(multi_hash.value())};
      cid_ = std::move(id);
    }
    return cid_.value();
  }

  const common::Buffer &NodeImpl::getRawBytes() const {
    return getCachePB();
  }

  void NodeImpl::addLink(const Link &link) {
    auto &link_impl = dynamic_cast<const LinkImpl &>(link);
    links_.emplace(link.getName(), link_impl);
  }

  std::vector<std::reference_wrapper<const Link>> NodeImpl::getLinks() const {
    std::vector<std::reference_wrapper<const Link>> link_refs{};
    for (const auto &link : links_) {
      link_refs.emplace_back(link.second);
    }
    return link_refs;
  }

  std::shared_ptr<Node> NodeImpl::createFromString(const std::string &content) {
    std::vector<uint8_t> data{content.begin(), content.end()};
    auto node = std::make_shared<NodeImpl>();
    node->assign(common::Buffer{data});
    return node;
  }

  outcome::result<std::shared_ptr<Node>> NodeImpl::createFromRawBytes(
      gsl::span<const uint8_t> input) {
    PBNodeDecoder decoder;
    if (auto result = decoder.decode(input); result.has_error()) {
      return result.error();
    }
    auto node = createFromString(decoder.getContent());
    for (size_t i = 0; i < decoder.getLinksCount(); ++i) {
      std::vector<uint8_t> link_cid_bytes{decoder.getLinkCID(i).begin(),
                                          decoder.getLinkCID(i).end()};
      OUTCOME_TRY(link_cid, ContentIdentifierCodec::decode(link_cid_bytes));
      LinkImpl link{
          std::move(link_cid), decoder.getLinkName(i), decoder.getLinkSize(i)};
      node->addLink(link);
    }
    return node;
  }

  const common::Buffer &NodeImpl::getCachePB() const {
    if (!pb_cache_) {
      pb_cache_ = PBNodeEncoder::encode(content_, links_);
    }
    return pb_cache_.value();
  }
}  // namespace fc::storage::ipfs::merkledag

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs::merkledag, NodeError, e) {
  using fc::storage::ipfs::merkledag::NodeError;
  switch (e) {
    case (NodeError::LINK_NOT_FOUND):
      return "MerkleDAG Node: link not exist";
    case (NodeError::INVALID_RAW_DATA):
      return "MerkleDAG Node: failed to deserialize from incorrect raw bytes";
  }
  return "MerkleDAG Node: unknown error";
}
