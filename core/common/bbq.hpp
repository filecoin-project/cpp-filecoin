/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_BLOCKING_QUEUE_HPP
#define CPP_FILECOIN_CORE_COMMON_BLOCKING_QUEUE_HPP

#include <atomic>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>

#include <boost/assert.hpp>
#include <boost/optional.hpp>

namespace fc::common {

  /**
   * @brief interface for receiving party of the queue
   */
  template <class T>
  class BufferedBlockingReceiver {
   public:
    virtual ~BufferedBlockingReceiver() = default;

    virtual boost::optional<T> pop() = 0;

    virtual void close() = 0;
  };

  /**
   * @brief interface for transmitting party of the queue
   */
  template <class T>
  class BufferedBlockingTransmitter {
   public:
    virtual ~BufferedBlockingTransmitter() = default;

    virtual bool push(T &&value) = 0;

    virtual void close() = 0;
  };

  /**
   * @brief implements buffered one-directional blocking queue
   * is designed for using together with producer/consumer paradigm
   * you need two or more threads in order to divide producers from consumers
   * producers should use only transmitter interface
   * consumer should use only receiver interface
   * otherwise deadlocks can occur
   */
  template <class T>
  class BufferedBlockingQueue
      : public BufferedBlockingReceiver<T>,
        public BufferedBlockingTransmitter<T>,
        public std::enable_shared_from_this<BufferedBlockingQueue<T>> {
   public:
    ~BufferedBlockingQueue() override = default;

    explicit BufferedBlockingQueue(size_t size = 1u) : blocking_size_{size} {
      BOOST_ASSERT_MSG(size > 0, "empty queue is not allowed");
    }

    std::weak_ptr<BufferedBlockingReceiver<T>> getReceiver() {
      return this->weak_from_this();
    }

    std::weak_ptr<BufferedBlockingTransmitter<T>> getTransmitter() {
      return this->weak_from_this();
    }

    boost::optional<T> pop() override {
      std::unique_lock<std::mutex> lock(read_mutex_);
      read_sync_.wait(lock, [this]() {
        return read_available_.load() || is_closed_.load();
      });

      if (is_closed_.load()) {
        return boost::none;
      }
      auto value = boost::optional<T>(std::move(items_.front()));
      items_.pop_front();
      read_available_.store(items_.size() > 0);
      write_available_.store(true);
      write_sync_.notify_one();
      return value;
    }

    bool push(T &&value) override {
      std::unique_lock<std::mutex> lock(write_mutex_);
      write_sync_.wait(
          lock, [this]() { return write_available_.load() || is_closed_; });
      if (is_closed_) return false;

      items_.push_back(value);
      write_available_.store(items_.size() < blocking_size_);
      read_available_.store(true);
      read_sync_.notify_one();
      return true;
    }

    void close() override {
      is_closed_.store(true);
      read_sync_.notify_all();
      write_sync_.notify_all();
    }

   private:
    std::atomic<size_t> blocking_size_{};
    std::mutex read_mutex_;
    std::mutex write_mutex_;
    std::atomic_bool is_closed_{false};
    std::atomic_bool read_available_{false};
    std::atomic_bool write_available_{true};
    std::condition_variable read_sync_;
    std::condition_variable write_sync_;
    std::deque<T> items_;
  };
}  // namespace fc::common

#endif  // CPP_FILECOIN_CORE_COMMON_BLOCKING_QUEUE_HPP
