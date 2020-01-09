/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_BLOCKSERVICE_HPP
#define FILECOIN_STORAGE_IPFS_BLOCKSERVICE_HPP

#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "storage/ipfs/datastore.hpp"

#include "storage/ipfs/block.hpp"

namespace fc::storage::ipfs {
  /**
   * @interface Provides a seamless interface to both local and remote storage
   * backends
   */
  class BlockService {
   public:
    /**
     * @brief Default destructor
     */
    virtual ~BlockService() = default;

    /**
     * @brief Add new block to local storage
     * @param block - entity to add
     * @return operation result
     */
    virtual outcome::result<void> addBlock(const Block &block) = 0;

    /**
     * @brief Check for block existence in the local and remote storage
     * @param cid - block id
     * @return operation result
     */
    virtual outcome::result<bool> has(const CID &cid) const = 0;

    /**
     * @brief Get block's content from local or remote storage
     * @param cid - block id
     * @return operation result
     */
    virtual outcome::result<Block::Content> getBlockContent(
        const CID &cid) const = 0;

    /**
     * @brief Remove block from local storage
     * @param cid - block id
     * @return operation result
     */
    virtual outcome::result<void> removeBlock(const CID &cid) = 0;
  };

  /**
   * @enum Block service errors
   */
  enum class BlockServiceError : int {
    CID_NOT_FOUND = 1,
    ADD_BLOCK_FAILED,
    GET_BLOCK_FAILED,
    REMOVE_BLOCK_FAILED
  };
}  // namespace fc::storage::ipfs

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs, BlockServiceError);

#endif
