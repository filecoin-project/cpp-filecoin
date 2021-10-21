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
      current_iterator_ = invalid_;
    };

    void seekToFirst() override {
      current_iterator_ = storage_->storage.begin();
      if (current_iterator_ == storage_->storage.end()) {
        current_iterator_ = invalid_;
      }
    }

    void seek(const Bytes &key) override {
      current_iterator_ = storage_->storage.lower_bound(key);
    }

    void seekToLast() override {
      current_iterator_ = storage_->storage.end();
      if (current_iterator_ == storage_->storage.begin()) {
        current_iterator_ = invalid_;
      } else {
        --current_iterator_;
      }
    }

    bool isValid() const override {
      return current_iterator_ != invalid_;
    }

    void next() override {
      assert(isValid());
      current_iterator_++;
      if (current_iterator_ == storage_->storage.end()) {
        current_iterator_ = invalid_;
      }
    }

    void prev() override {
      assert(isValid());
      if (current_iterator_ == storage_->storage.begin()) {
        current_iterator_ = invalid_;
      } else {
        current_iterator_--;
      }
    }

    Bytes key() const override {
      return current_iterator_->first;
    }

    Bytes value() const override {
      return current_iterator_->second;
    }

   private:
    std::map<Bytes, Bytes>::iterator current_iterator_, invalid_;

    std::shared_ptr<InMemoryStorage> storage_;
  };

}  // namespace fc::storage
