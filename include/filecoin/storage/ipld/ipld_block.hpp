/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPLD_BLOCK_HPP
#define FILECOIN_STORAGE_IPLD_BLOCK_HPP

#include <vector>

#include "filecoin/common/buffer.hpp"
#include "filecoin/primitives/cid/cid.hpp"

namespace fc::storage::ipld {
  /**
   * @class Interface for various data structure, linked with CID
   */
  class IPLDBlock {
   public:
    /**
     * @brief Destructor
     */
    virtual ~IPLDBlock() = default;

    /**
     * @brief Get CID of the data structure
     * @return IPLD identificator
     */
    virtual const CID &getCID() const = 0;

    /**
     * @brief Get data structure bytes from cache or generate new
     * @return Serialized value
     */
    virtual const common::Buffer &getRawBytes() const = 0;

  };
}  // namespace fc::storage::ipld

#endif
