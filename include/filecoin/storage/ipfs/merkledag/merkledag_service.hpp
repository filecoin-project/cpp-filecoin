/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_MERKLEDAG_SERVICE_HPP
#define FILECOIN_STORAGE_IPFS_MERKLEDAG_SERVICE_HPP

#include <memory>

#include "filecoin/common/outcome.hpp"
#include "filecoin/storage/ipfs/merkledag/leaf.hpp"
#include "filecoin/storage/ipld/ipld_node.hpp"

namespace fc::storage::ipfs::merkledag {
  using ipld::IPLDNode;

  class MerkleDagService {
   public:
    /**
     * @brief Destructor
     */
    virtual ~MerkleDagService() = default;

    /**
     * @brief Add new node
     * @param node - entity to add
     * @return operation result
     */
    virtual outcome::result<void> addNode(std::shared_ptr<const IPLDNode> node) = 0;

    /**
     * @brief Get node by id
     * @param cid - node id
     * @return operation result
     */
    virtual outcome::result<std::shared_ptr<IPLDNode>> getNode(
        const CID &cid) const = 0;

    /**
     * @brief Remove node by id
     * @param cid - node id
     * @return operation result
     */
    virtual outcome::result<void> removeNode(const CID &cid) = 0;

    /**
     * @brief Get nodes with IPLD-selector
     * @param root_cid - bytes of the root Node CID
     * @param selector - IPLD-selector raw bytes
     * @param handler - receiver of the selected Nodes, should return false to
     * break receiving process
     * @return count of the received by handler Nodes
     */
    virtual outcome::result<size_t> select(
        gsl::span<const uint8_t> root_cid,
        gsl::span<const uint8_t> selector,
        std::function<bool(std::shared_ptr<const IPLDNode> node)> handler)
        const = 0;

    /**
     * @brief Fetcg graph from given root node
     * @param cid - identifier of the root node
     * @return operation result
     */
    virtual outcome::result<std::shared_ptr<Leaf>> fetchGraph(
        const CID &cid) const = 0;

    /**
     * @brief Construct graph from given root node till chosen limit
     *        (0 means "Fetch only root node")
     * @param cid - identifier of the root node
     * @param depth - limit of the depth to fetch
     * @return operation result
     */
    virtual outcome::result<std::shared_ptr<Leaf>> fetchGraphOnDepth(
        const CID &cid, uint64_t depth) const = 0;
  };

  /**
   * @class Possible MerkleDAG service errors
   */
  enum class ServiceError {
    UNRESOLVED_LINK = 1  // This error can occur if child node not found
  };
}  // namespace fc::storage::ipfs::merkledag

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs::merkledag, ServiceError)

#endif
