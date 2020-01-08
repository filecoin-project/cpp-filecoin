/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/impl/blockservice_impl.hpp"

using fc::storage::ipfs::Block;

namespace fc::storage::ipfs {
  BlockServiceImpl::BlockServiceImpl(std::shared_ptr<IpfsDatastore> data_store)
      : local_storage_{std::move(data_store)} {
    BOOST_ASSERT_MSG(local_storage_ != nullptr,
                     "BlockService: IpfsDatastore does not exist");
  }

  outcome::result<void> BlockServiceImpl::addBlock(const Block &block) {
    auto result = local_storage_->set(block.getCID(), block.getContent());
    if (result.has_error()) {
      return BlockServiceError::ADD_BLOCK_FAILED;
    }
    return outcome::success();
  }

  outcome::result<bool> BlockServiceImpl::has(const CID &cid) const {
    return local_storage_->contains(cid);
  }

  outcome::result<Block::Content> BlockServiceImpl::getBlockContent(
      const CID &cid) const {
    outcome::result<Block::Content> data = local_storage_->get(cid);
    if (data.has_error()) {
      if (data.error() == IpfsDatastoreError::NOT_FOUND) {
        return BlockServiceError::CID_NOT_FOUND;
      }
      return BlockServiceError::GET_BLOCK_FAILED;
    }
    return data;
  }

  outcome::result<void> BlockServiceImpl::removeBlock(const CID &cid) {
    auto result = local_storage_->remove(cid);
    if (result.has_error()) {
      if (result.error() == IpfsDatastoreError::NOT_FOUND) {
        return BlockServiceError::CID_NOT_FOUND;
      }
      return BlockServiceError::REMOVE_BLOCK_FAILED;
    }
    return outcome::success();
  }
}  // namespace fc::storage::ipfs

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs, BlockServiceError, e) {
  using fc::storage::ipfs::BlockServiceError;
  switch (e) {
    case BlockServiceError::CID_NOT_FOUND:
      return "BlockServiceError: block with given CID not found";
    case BlockServiceError::ADD_BLOCK_FAILED:
      return "BlockServiceError: failed to add block to datastore";
    case BlockServiceError::GET_BLOCK_FAILED:
      return "BlockServiceError: failed to get block from datastore";
    case BlockServiceError::REMOVE_BLOCK_FAILED:
      return "BlockServiceError: failed to remove block from datastore";
  }
  return "BlockServiceError: unknown error";
}
