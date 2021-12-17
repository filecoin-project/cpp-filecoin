/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/compacter/queue.hpp"

#include <boost/filesystem/operations.hpp>

#include "codec/cbor/light_reader/cid.hpp"
#include "common/error_text.hpp"
#include "common/file.hpp"

namespace fc::storage::compacter {
  inline void _error() {
    outcome::raise(ERROR_TEXT(__FILE__));
  }

  void CompacterQueue::open(bool clear) {
    std::unique_lock lock{mutex};
    stack.clear();
    std::ifstream reader{path};
    if (!clear && reader) {
      auto tmp_path{path + ".tmp"};
      std::ofstream tmp_file{tmp_path};
      CbCid key;
      while (common::readStruct(reader, key)) {
        if (!visited->has(key)) {
          if (!common::writeStruct(tmp_file, key)) {
            _error();
          }
          stack.push_back(key);
        }
      }
      if (!tmp_file.flush()) {
        _error();
      }
      tmp_file.close();
      boost::filesystem::rename(tmp_path, path);
      writer.open(path, std::ios::app);
    } else {
      writer.open(path);
    }
  }

  void CompacterQueue::clear() {
    std::unique_lock lock{mutex};
    stack.clear();
    writer.close();
    boost::filesystem::remove(path);
  }

  bool CompacterQueue::_push(const CbCid &key) {
    if (visited->has(key)) {
      return false;
    }
    if (!common::writeStruct(writer, key)) {
      _error();
    }
    stack.push_back(key);
    return true;
  }

  void CompacterQueue::push(const CbCid &key) {
    std::unique_lock lock{mutex};
    if (_push(key) && !writer.flush()) {
      _error();
    }
  }

  void CompacterQueue::push(const std::vector<CbCid> &keys) {
    std::unique_lock lock{mutex};
    auto any{false};
    for (const auto &key : keys) {
      if (_push(key)) {
        any = true;
      }
    }
    if (any && !writer.flush()) {
      _error();
    }
  }

  void CompacterQueue::pushChildren(BytesIn input) {
    std::unique_lock lock{mutex};
    BytesIn cid;
    auto any{false};
    while (codec::cbor::findCid(cid, input)) {
      const CbCid *key = nullptr;
      if (codec::cbor::light_reader::readCborBlake(key, cid)) {
        if (_push(*key)) {
          any = true;
        }
      }
    }
    if (any && !writer.flush()) {
      _error();
    }
  }

  bool CompacterQueue::empty() {
    std::unique_lock lock{mutex};
    return stack.empty();
  }

  std::optional<CbCid> CompacterQueue::pop() {
    std::unique_lock lock{mutex};
    while (true) {
      if (stack.empty()) {
        return {};
      }
      auto key{stack.back()};
      stack.pop_back();
      if (!visited->has(key)) {
        return key;
      }
    }
  }
}  // namespace fc::storage::compacter
