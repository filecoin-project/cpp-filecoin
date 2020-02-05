/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_BLOCK_HPP
#define FILECOIN_STORAGE_IPFS_BLOCK_HPP

#include <functional>

#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::storage::ipfs {
  /**
   * @interface Piece of data, which is used with BlockService
   */
  struct Block {
    using Content = common::Buffer; /**< Alias for content type */

    /**
     * @brief Default destructor
     */
    virtual ~Block() = default;

    /**
     * @brief Get content identifier
     * @return Block CID
     */
    virtual const CID &getCID() const = 0;

    /**
     * @brief Get block content
     * @return Block's raw data for store in the BlockService
     */
    virtual const Content &getRawBytes() const = 0;
  };
}  // namespace fc::storage::ipfs

#endif
