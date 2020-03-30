/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipld/impl/ipld_node_impl.hpp"

#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/multi/multibase_codec/codecs/base58.hpp>
#include "ipld_node.pb.h"
#include "storage/ipld/impl/ipld_node_decoder_pb.hpp"

using libp2p::common::Hash256;
using libp2p::multi::ContentIdentifierCodec;
using libp2p::multi::HashType;
using libp2p::multi::MulticodecType;
using libp2p::multi::Multihash;
using protobuf::ipld::node::PBLink;
using protobuf::ipld::node::PBNode;
using Version = libp2p::multi::ContentIdentifier::Version;

namespace fc::storage::ipld {

  const CID &IPLDNodeImpl::getCID() const {
    return getIPLDBlock().cid;
  }

  const IPLDNode::Buffer &IPLDNodeImpl::getRawBytes() const {
    return getIPLDBlock().bytes;
  }

  size_t IPLDNodeImpl::size() const {
    return getRawBytes().size() + child_nodes_size_;
  }

  void IPLDNodeImpl::assign(common::Buffer input) {
    content_ = std::move(input);
    ipld_block_ =
        boost::none;  // Need to recalculate CID after changing Node content
  }

  const common::Buffer &IPLDNodeImpl::content() const {
    return content_;
  }

  outcome::result<void> IPLDNodeImpl::addChild(
      const std::string &name, std::shared_ptr<const IPLDNode> node) {
    IPLDLinkImpl link{node->getCID(), name, node->size()};
    links_.emplace(name, std::move(link));
    ipld_block_ =
        boost::none;  // Need to recalculate CID after adding link to child node
    child_nodes_size_ += node->size();
    return outcome::success();
  }

  outcome::result<std::reference_wrapper<const IPLDLink>> IPLDNodeImpl::getLink(
      const std::string &name) const {
    if (auto index = links_.find(name); index != links_.end()) {
      return index->second;
    }
    return IPLDNodeError::LINK_NOT_FOUND;
  }

  void IPLDNodeImpl::removeLink(const std::string &link_name) {
    if (auto index = links_.find(link_name); index != links_.end()) {
      child_nodes_size_ -= index->second.getSize();
      links_.erase(index);
    }
  }

  void IPLDNodeImpl::addLink(const IPLDLink &link) {
    auto &link_impl = dynamic_cast<const IPLDLinkImpl &>(link);
    links_.emplace(link.getName(), link_impl);
  }

  std::vector<std::reference_wrapper<const IPLDLink>> IPLDNodeImpl::getLinks()
      const {
    std::vector<std::reference_wrapper<const IPLDLink>> link_refs{};
    for (const auto &link : links_) {
      link_refs.emplace_back(link.second);
    }
    return link_refs;
  }

  IPLDNode::Buffer IPLDNodeImpl::serialize() const {
    return Buffer{IPLDNodeEncoderPB::encode(content_, links_)};
  }

  std::shared_ptr<IPLDNode> IPLDNodeImpl::createFromString(
      const std::string &content) {
    std::vector<uint8_t> data{content.begin(), content.end()};
    auto node = std::make_shared<IPLDNodeImpl>();
    node->assign(common::Buffer{data});
    return node;
  }

  outcome::result<std::shared_ptr<IPLDNode>> IPLDNodeImpl::createFromRawBytes(
      gsl::span<const uint8_t> input) {
    IPLDNodeDecoderPB decoder;
    if (auto result = decoder.decode(input); result.has_error()) {
      return result.error();
    }
    auto node = createFromString(decoder.getContent());
    for (size_t i = 0; i < decoder.getLinksCount(); ++i) {
      std::vector<uint8_t> link_cid_bytes{decoder.getLinkCID(i).begin(),
                                          decoder.getLinkCID(i).end()};
      OUTCOME_TRY(link_cid, ContentIdentifierCodec::decode(link_cid_bytes));
      IPLDLinkImpl link{
          std::move(link_cid), decoder.getLinkName(i), decoder.getLinkSize(i)};
      node->addLink(link);
    }
    return node;
  }

  const IPLDBlock &IPLDNodeImpl::getIPLDBlock() const {
    if (!ipld_block_) {
      ipld_block_ = IPLDBlock::create(*this);
    }
    return ipld_block_.value();
  }
}  // namespace fc::storage::ipld

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipld, IPLDNodeError, e) {
  using fc::storage::ipld::IPLDNodeError;
  switch (e) {
    case (IPLDNodeError::LINK_NOT_FOUND):
      return "MerkleDAG Node: link not exist";
    case (IPLDNodeError::INVALID_RAW_DATA):
      return "MerkleDAG Node: failed to deserialize from incorrect raw bytes";
  }
  return "MerkleDAG Node: unknown error";
}
