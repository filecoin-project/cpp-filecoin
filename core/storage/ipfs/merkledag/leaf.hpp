/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_MERKLEDAG_LEAF_HPP
#define FILECOIN_STORAGE_IPFS_MERKLEDAG_LEAF_HPP

#include <functional>
#include <string_view>

#include "common/buffer.hpp"
#include "common/outcome.hpp"

namespace fc::storage::ipfs::merkledag {
  class Leaf {
   public:
    /**
     * @brief Destructor
     */
    virtual ~Leaf() = default;

    /**
     * @brief Get leaf content
     * @return operation result
     */
    virtual const common::Buffer &content() const = 0;

    /**
     * @brief Get count of children leaves
     * @return Count of leaves
     */
    virtual size_t count() const = 0;

    /**
     * @brief Get children leaf
     * @param name - leaf name
     * @return operation result
     */
    virtual outcome::result<std::reference_wrapper<const Leaf>> subLeaf(
        std::string_view name) const = 0;

    /**
     * @brief Get names of all sub leaves of the current leaf
     * @return operation result
     */
    virtual std::vector<std::string_view> getSubLeafNames() const = 0;
  };

  /**
   * @enum Possible leaf errors
   */
  enum class LeafError : int { LEAF_NOT_FOUND = 1, DUPLICATE_LEAF };
}  // namespace fc::storage::ipfs::merkledag

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs::merkledag, LeafError)

#endif
