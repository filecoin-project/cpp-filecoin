/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/in_memory/in_memory_storage.hpp"

namespace fc::storage {

  /**
   * @brief Instance of cursor can be used as bidirectional iterator over
   * key-value bindings of the Map.
   */
  class InMemoryCursor : public BufferMapCursor {
   public:
    InMemoryCursor(std::shared_ptr<InMemoryStorage> storage)
        : storage_(std::move(storage)) {
      current_iterator_ = storage_->storage.end();
    };

    void seekToFirst() override {
      current_iterator_ = storage_->storage.begin();
    }

    void seek(const Buffer &key) override {
      current_iterator_ = storage_->storage.find(key.toHex());
    }

    void seekToLast() override {
      current_iterator_ = (storage_->storage.rbegin()++).base();
    }

    bool isValid() const override {
      return current_iterator_ != storage_->storage.end();
    }

    void next() override {
      current_iterator_++;
    }

    void prev() override {
      current_iterator_--;
    }

    Buffer key() const override {
      OUTCOME_EXCEPT(key, Buffer::fromHex(current_iterator_->first));
      return key;
    }

    Buffer value() const override {
      return current_iterator_->second;
    }

   private:
    std::map<std::string, Buffer>::iterator current_iterator_;

    std::shared_ptr<InMemoryStorage> storage_;
  };

}  // namespace fc::storage
