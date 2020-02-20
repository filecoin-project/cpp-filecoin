/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include "blockchain/message_pool/message_storage.hpp"
#include "storage/ipfs/datastore.hpp"
#include "primitives/tipset/tipset.hpp"
#include "primitives/block/block.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::blockchain::production {
  class BlockGenerator {
    using fc::blockchain::message_pool::MessageStorage;
    using fc::primitives::tipset::Tipset;
    using fc::storage::ipfs::IpfsDataStore;
    using primitives::block::BlockHeader;

   public:
    BlockGenerator(std::shared_ptr<IpfsDataStore> data_store,
                   std::shared_ptr<MessageStorage> messages_store);


  };
}  // namespace fc::blockchain::production
