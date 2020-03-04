/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPLD_NODE_HPP
#define FILECOIN_STORAGE_IPLD_NODE_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "storage/ipld/ipld_block.hpp"
#include "storage/ipld/ipld_link.hpp"

namespace fc::storage::ipld {
  /**
   * @interface MerkleDAG service node
   */
  class IPLDNode : public virtual IPLDBlock {
   public:
    virtual ~IPLDNode() = default;
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
    virtual outcome::result<void> addChild(
        const std::string &name, std::shared_ptr<const IPLDNode> node) = 0;

    /**
     * @brief Get particular link to the child node
     * @param name - id of the link
     * @return Requested link of error, if link not found
     */
    virtual outcome::result<std::reference_wrapper<const IPLDLink>> getLink(
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
    virtual void addLink(const IPLDLink &link) = 0;

    virtual std::vector<std::reference_wrapper<const IPLDLink>> getLinks()
        const = 0;
  };

  /**
   * @class Possible Node errors
   */
  enum class IPLDNodeError { LINK_NOT_FOUND = 1, INVALID_RAW_DATA };
}  // namespace fc::storage::ipld

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipld, IPLDNodeError)

#endif
