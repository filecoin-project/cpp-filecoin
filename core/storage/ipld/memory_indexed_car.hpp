/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fstream>
#include <map>
#include <mutex>

#include "storage/ipfs/datastore.hpp"

namespace fc {
  struct MemoryIndexedCar : Ipld {
    mutable std::mutex mutex;
    mutable std::ifstream reader;
    std::ofstream writer;
    uint64_t end{};
    std::vector<CID> roots;
    // value offset and size
    std::map<CID, std::pair<uint64_t, size_t>> index;

    static outcome::result<std::shared_ptr<MemoryIndexedCar>> make(
        const std::string &path, bool append);

    outcome::result<bool> contains(const CID &key) const override;
    outcome::result<void> set(const CID &key, Bytes value) override;
    outcome::result<void> set(const CID &key, BytesIn value) override;
    outcome::result<Bytes> get(const CID &key) const override;
  };
}  // namespace fc
