/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_MERKLEDAG_LINK_HPP
#define FILECOIN_STORAGE_IPFS_MERKLEDAG_LINK_HPP

#include <string>
#include <vector>

#include "primitives/cid/cid.hpp"

namespace fc::storage::ipfs::merkledag {
  class Link {
   public:
    /**
     * @brief Destructor
     */
    virtual ~Link() = default;

    /**
     * @brief Get name of the link
     * @return Name, which should be unique per object
     */
    virtual const std::string &getName() const = 0;

    /**
     * @brief Get identifier of the target object
     * @return Content identifier
     */
    virtual const CID &getCID() const = 0;

    /**
     * @brief Get target object size
     * @return Cumulative size of the target object
     */
    virtual size_t getSize() const = 0;
  };
}  // namespace fc::storage::ipfs::merkledag

#endif
