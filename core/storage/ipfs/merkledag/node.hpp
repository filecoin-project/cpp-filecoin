/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_MERKLEDAG_NODE_HPP
#define FILECOIN_STORAGE_IPFS_MERKLEDAG_NODE_HPP

#include <functional>
#include <memory>
#include <vector>
#include <string>

#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "storage/ipfs/ipfs_block.hpp"
#include "storage/ipfs/merkledag/link.hpp"

namespace fc::storage::ipfs::merkledag {
  /**
   * @interface MerkleDAG service node
   */
  class Node : public IpfsBlock {
   public:
    /**
     * @brief Total size of the data including the total sizes of references
     * @return Cumulative size in bytes
     */
    virtual size_t size() const = 0;

    /**
     * @brief Assign Node's content
     * @param input - data bytes
     * @return operation result
     */
    virtual void assign(common::Buffer input) = 0;

    /**
     * @brief Get Node data
     * @return content bytes
     */
    virtual const common::Buffer &content() const = 0;

    /**
     * @brief Add link to the child node
     * @param name - name of the child node
     * @param node - child object to link
     * @return operation result
     */
    virtual outcome::result<void> addChild(const std::string &name,
                                           std::shared_ptr<const Node> node) = 0;

    /**
     * @brief Get particular link to the child node
     * @param name - id of the link
     * @return Requested link of error, if link not found
     */
    virtual outcome::result<std::reference_wrapper<const Link>> getLink(
        const std::string &name) const = 0;

    /**
     * @brief Remove link to the child node
     * @param name - name of the child node
     * @return operation result
     */
    virtual void removeLink(const std::string &name) = 0;

    /**
     * @brief Insert link to the child node
     * @param link - object to add
     */
    virtual void addLink(const Link &link) = 0;

    virtual std::vector<std::reference_wrapper<const Link>> getLinks()
        const = 0;
  };

  /**
   * @class Possible Node errors
   */
  enum class NodeError : int { LINK_NOT_FOUND, INVALID_RAW_DATA };
}  // namespace fc::storage::ipfs::merkledag

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs::merkledag, NodeError)

#endif
