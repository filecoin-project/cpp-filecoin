/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fstream>
#include <mutex>
#include <optional>

#include "cbor_blake/ipld.hpp"

namespace fc::storage::compacter {
  struct CompacterQueue {
    void open(bool clear);
    void clear();
    bool _push(const CbCid &key);
    void push(const CbCid &key);
    void push(const std::vector<CbCid> &keys);
    void pushChildren(BytesIn input);
    bool empty();
    std::optional<CbCid> pop();

    std::string path;
    CbIpldPtr visited;

    std::mutex mutex;
    std::vector<CbCid> stack;
    std::ofstream writer;
  };
}  // namespace fc::storage::compacter
