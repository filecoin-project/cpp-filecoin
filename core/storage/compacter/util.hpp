/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cbor_blake/ipld_any.hpp"
#include "storage/compacter/compacter.hpp"

namespace fc::storage::compacter {
  inline auto make(std::string path,
                   MapPtr kv,
                   std::shared_ptr<CidsIpld> old_ipld,
                   SharedMutexPtr ts_mutex) {
    auto compacter{std::make_shared<CompacterIpld>()};
    compacter->path = path;
    compacter->old_ipld = old_ipld;
    compacter->ts_mutex = ts_mutex;
    compacter->start_head_key = {"compacter_start_head", kv};
    compacter->headers_top_key = {"compacter_headers_top", kv};
    compacter->queue = std::make_shared<CompacterQueue>();
    compacter->queue->path = path + ".queue";
    compacter->interpreter = std::make_shared<CompacterInterpreter>();
    compacter->interpreter->mutex = std::make_shared<std::shared_mutex>();
    compacter->put_block_header = std::make_shared<CompacterPutBlockHeader>();
    compacter->put_block_header->_compacter = compacter;
    return compacter;
  }
}  // namespace fc::storage::compacter
