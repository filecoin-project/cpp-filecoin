/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_LRU_CACHE_HPP
#define CPP_FILECOIN_SYNC_LRU_CACHE_HPP

#include <cassert>
#include <functional>
#include <map>
#include <memory>

namespace fc::sync {

  template <typename Key, typename Value>
  class LRUCache {
    static constexpr size_t kNpos = -1;

   public:
    using ExtractKey = std::function<Key(const Value &)>;

    LRUCache(size_t size_limit, ExtractKey extract_key_fn)
        : size_limit_(size_limit), extract_key_(std::move(extract_key_fn)) {
      assert(size_limit_ >= 1);
      assert(extract_key_);
      items_.reserve(size_limit_);
    }

    std::shared_ptr<const Value> get(const Key &key) {
      auto it = map_.find(key);
      if (it == map_.end()) {
        return std::shared_ptr<const Value>{};
      }
      size_t pos = it->second;
      auto &item = items_[pos];
      bringToFront(item, pos);
      return item.value;
    }

    void modifyValues(std::function<void(Value &value)> cb) {
      for (auto &item : items_) {
        cb(*(item.value));
      }
    }

    void put(std::shared_ptr<Value> value, bool update_if_exists) {
      assert(value);
      assert(items_.size() == map_.size());

      auto key = extract_key_(*value);
      auto it = map_.find(key);
      if (it == map_.end()) {
        if (items_.size() >= size_limit_) {
          assert(lru_last_ < items_.size());
          auto &item = items_[lru_last_];
          bringToFront(item, lru_last_);
          map_.erase(extract_key_(*item.value));
          item.value = std::move(value);
        } else {
          size_t pos = items_.size();
          Item new_item{std::move(value), kNpos, lru_first_};
          if (pos == 0) {
            lru_last_ = pos;
          } else {
            items_[lru_first_].prev = pos;
          }
          lru_first_ = pos;
          items_.push_back(std::move(new_item));
        }
        map_[key] = lru_first_;
      } else {
        auto pos = it->second;
        auto &item = items_[pos];
        bringToFront(item, pos);
        if (update_if_exists) {
          item.value = std::move(value);
        }
      }
    }

   private:
    struct Item {
      std::shared_ptr<Value> value;
      size_t prev = kNpos;
      size_t next = kNpos;
    };

    // TODO (artem): make std::hash out of TipsetHash and use unordered
    using Map = std::map<Key, size_t>;

    void unlinkItem(Item &item) {
      if (item.next != kNpos) {
        items_[item.next].prev = item.prev;
      } else {
        lru_last_ = item.prev;
      }
      if (item.prev != kNpos) {
        items_[item.prev].next = item.next;
      } else {
        lru_first_ = item.next;
      }
    }

    void bringToFront(Item& item, size_t pos) {
      assert(pos < items_.size());
      assert(lru_first_ < items_.size());
      if (pos != lru_first_) {
        assert(item.prev != kNpos);
        unlinkItem(item);
        Item &first = items_[lru_first_];
        assert(first.prev == kNpos);
        item.prev = kNpos;
        item.next = lru_first_;
        first.prev = pos;
        lru_first_ = pos;
      }
    }

    const size_t size_limit_;
    ExtractKey extract_key_;
    std::vector<Item> items_;
    size_t lru_first_ = kNpos;
    size_t lru_last_ = kNpos;
    Map map_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_LRU_CACHE_HPP
