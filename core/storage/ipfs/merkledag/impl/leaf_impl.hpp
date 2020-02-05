/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_MERKLEDAG_LEAF_IMPL_HPP
#define FILECOIN_STORAGE_IPFS_MERKLEDAG_LEAF_IMPL_HPP

#include "storage/ipfs/merkledag/leaf.hpp"

#include <map>
#include <string>

namespace fc::storage::ipfs::merkledag {
  class LeafImpl : public Leaf {
   public:
    /**
     * @brief Construct leaf
     * @param data - leaf content
     */
    explicit LeafImpl(common::Buffer data);

    const common::Buffer &content() const override;

    size_t count() const override;

    outcome::result<std::reference_wrapper<const Leaf>> subLeaf(
        std::string_view name) const override;

    std::vector<std::string_view> getSubLeafNames() const override;

    /**
     * @brief Insert children leaf
     * @param name - id of the leaf
     * @param children - leaf to insert
     * @return operation result
     */
    outcome::result<void> insertSubLeaf(std::string name, LeafImpl children);

   private:
    common::Buffer content_;
    std::map<std::string, LeafImpl, std::less<>> children_;
  };
}  // namespace fc::storage::ipfs::merkledag

#endif
