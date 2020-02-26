/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPLD_NODE_IMPL_HPP
#define FILECOIN_STORAGE_IPLD_NODE_IMPL_HPP

#include <map>
#include <memory>
#include <string_view>
#include <vector>

#include <boost/optional.hpp>
#include "storage/ipld/impl/ipld_block_impl.hpp"
#include "storage/ipld/impl/ipld_link_impl.hpp"
#include "storage/ipld/impl/ipld_node_encoder_pb.hpp"
#include "storage/ipld/ipld_node.hpp"

namespace fc::storage::ipld {
  class IPLDNodeImpl : public IPLDNode, IPLDBlockImpl {
   public:
    IPLDNodeImpl();

    size_t size() const override;

    void assign(common::Buffer input) override;

    const common::Buffer &content() const override;

    outcome::result<void> addChild(
        const std::string &name, std::shared_ptr<const IPLDNode> node) override;

    outcome::result<std::reference_wrapper<const IPLDLink>> getLink(
        const std::string &name) const override;

    void removeLink(const std::string &name) override;

    void addLink(const IPLDLink &link) override;

    std::vector<std::reference_wrapper<const IPLDLink>> getLinks()
        const override;

    static std::shared_ptr<IPLDNode> createFromString(
        const std::string &content);

    static outcome::result<std::shared_ptr<IPLDNode>> createFromRawBytes(
        gsl::span<const uint8_t> input);

   private:
    common::Buffer content_;
    std::map<std::string, IPLDLinkImpl> links_;
    IPLDNodeEncoderPB pb_node_codec_;
    size_t child_nodes_size_{};

    common::Buffer serialize() const override;
  };
}  // namespace fc::storage::ipld

#endif
