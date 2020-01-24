/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_MERKLEDAG_SERVICE_IMPL_HPP
#define FILECOIN_STORAGE_IPFS_MERKLEDAG_SERVICE_IMPL_HPP

#include <memory>

#include "storage/ipfs/blockservice.hpp"
#include "storage/ipfs/merkledag/impl/leave_impl.hpp"
#include "storage/ipfs/merkledag/service.hpp"

namespace fc::storage::ipfs::merkledag {
  class ServiceImpl : public Service {
   public:
    /**
     * @brief Construct service
     * @param service - underlying block service
     */
    explicit ServiceImpl(std::shared_ptr<BlockService> service);

    outcome::result<void> addNode(std::shared_ptr<const Node> node) override;

    outcome::result<std::shared_ptr<Node>> getNode(
        const CID &cid) const override;

    outcome::result<void> removeNode(const CID &cid) override;

    outcome::result<std::shared_ptr<Leave>> fetchGraph(
        const CID &cid) const override;

    outcome::result<std::shared_ptr<Leave>> fetchGraphOnDepth(
        const CID &cid, uint64_t depth) const override;

   private:
    std::shared_ptr<BlockService> block_service_;

    /**
     * @brief Fetch graph internal recursive implementation
     * @param root - leave, for which we should extract child nodes
     * @param links - links to the child nodes of this leave
     * @param depth_limit - status of the depth control
     * @param max_depth - e.g. "1" means "Fetch only root node with all
     * children, but without children of their children"
     * @param current_depth - value of the depth during current operation
     * @return operation result
     */
    outcome::result<void> buildGraph(
        const std::shared_ptr<LeaveImpl> &root,
        const std::vector<std::reference_wrapper<const Link>> &links,
        bool depth_limit,
        size_t max_depth,
        size_t current_depth = 0) const;
  };
}  // namespace fc::storage::ipfs::merkledag

#endif
