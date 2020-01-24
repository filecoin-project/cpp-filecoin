/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_MERKLEDAG_LINK_IMPL_HPP
#define FILECOIN_STORAGE_IPFS_MERKLEDAG_LINK_IMPL_HPP

#include <string>
#include <utility>
#include <vector>

#include "storage/ipfs/merkledag/link.hpp"

namespace fc::storage::ipfs::merkledag {
  class LinkImpl : public Link {
   public:
    /**
     * @brief Construct Link
     * @param id - CID of the target object
     * @param name - name of the target object
     * @param size - total size of the target object
     */
    LinkImpl(libp2p::multi::ContentIdentifier id,
             std::string name,
             size_t size);

    LinkImpl() = default;

    const std::string &getName() const override;

    const CID &getCID() const override;

    size_t getSize() const override;

   private:
    CID cid_;
    std::string name_;
    size_t size_{};
  };
}  // namespace fc::storage::ipfs::merkledag

#endif
