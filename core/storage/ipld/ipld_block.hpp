/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPLD_BLOCK_HPP
#define FILECOIN_STORAGE_IPLD_BLOCK_HPP

#include "common/buffer.hpp"
#include "primitives/cid/cid.hpp"

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

   protected:
    /**
     * @brief Optionally used for generating CID
     *        Children class must overload it to support structure serialization
     * @return Serialized value
     */
    virtual common::Buffer serialize() const {
      return {};
    }
  };
}  // namespace fc::storage::ipld

#endif
