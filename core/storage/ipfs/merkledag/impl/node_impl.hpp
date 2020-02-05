/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_MERKLEDAG_NODE_IMPL_HPP
#define FILECOIN_STORAGE_IPFS_MERKLEDAG_NODE_IMPL_HPP

#include <map>
#include <memory>
#include <string_view>
#include <vector>

#include <boost/optional.hpp>
#include "primitives/cid/cid.hpp"
#include "storage/ipfs/merkledag/impl/link_impl.hpp"
#include "storage/ipfs/merkledag/impl/pb_node_encoder.hpp"
#include "storage/ipfs/merkledag/node.hpp"

namespace fc::storage::ipfs::merkledag {
  class NodeImpl : public Node {
   public:

    size_t size() const override;

    void assign(common::Buffer input) override;

    const common::Buffer &content() const override;

    outcome::result<void> addChild(const std::string &name,
                                   std::shared_ptr<const Node> node) override;

    outcome::result<std::reference_wrapper<const Link>> getLink(
        const std::string &name) const override;

    void removeLink(const std::string &name) override;

    const CID &getCID() const override;

    const common::Buffer &getRawBytes() const override;

    void addLink(const Link &link) override;

    std::vector<std::reference_wrapper<const Link>> getLinks() const override;

    static std::shared_ptr<Node> createFromString(const std::string &content);

    static outcome::result<std::shared_ptr<Node>> createFromRawBytes(
        gsl::span<const uint8_t> input);

   private:
    mutable boost::optional<CID> cid_{};
    common::Buffer content_;
    std::map<std::string, LinkImpl> links_;
    PBNodeEncoder pb_node_codec_;
    size_t child_nodes_size_{};
    mutable boost::optional<common::Buffer> pb_cache_{boost::none};

    /**
     * @brief Check Protobuf-data cache status and generate new if needed
     * @return Actual protobuf-cache
     */
    const common::Buffer &getCachePB() const;
  };
}  // namespace fc::storage::ipfs::merkledag

#endif
