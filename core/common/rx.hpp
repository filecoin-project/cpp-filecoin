/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_RX_HPP
#define CPP_FILECOIN_CORE_COMMON_RX_HPP

#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <set>

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/optional.hpp>

namespace fc::common {

  /**
   * @author Artem Gorbachev
   * @brief typed async queue implementation
   * @class RX Receiver side of channel
   * @tparam T channel item type
   */
  template <class T>
  class RX : public std::enable_shared_from_this<RX<T>> {
   public:
    // Message callback, called from reactor thread
    using Callback = std::function<void(T &&message)>;

    // Ctor called by receiver side
    RX(std::shared_ptr<boost::asio::io_context> io, Callback &&callback)
        : io_(std::move(io)), callback_(std::move(callback)) {
      assert(io_);
      assert(callback_);
    }

    // Transmitter side of channel
    class TX {
     public:
      bool send(T &&message) {
        auto rx = rx_.lock();
        return rx ? rx->send(std::move(message)) : false;
      }

      bool send(const T &message) {
        auto rx = rx_.lock();
        return rx ? rx->send(message) : false;
      }

      size_t queue_size() {
        auto rx = rx_.lock();
        return rx ? rx->queue_size() : 0;
      }

      bool closed() {
        auto rx = rx_.lock();
        return rx ? rx->is_closed() : true;
      }

     private:
      template <class>
      friend class RX;  // friend because RX creates TX-es

      // Ctor called by RX, see friendship
      explicit TX(std::weak_ptr<RX<T>> rx) : rx_(std::move(rx)) {}

      // RX
      std::weak_ptr<RX<T>> rx_;
    };

    // RX creates TXes
    TX get_tx() {
      return TX(this->weak_from_this());
    }

    size_t queue_size() {
      std::lock_guard<std::mutex> lock(mutex_);
      return queue_.current_size();
    }

    boost::optional<T> receive() {
      std::lock_guard<std::mutex> lock(mutex_);
      posted_ = false;
      if (queue_.empty()) return boost::none;
      boost::optional<T> msg = std::move(queue_.front());
      queue_.pop_front();
      return msg;
    }

    bool is_closed() {
      std::lock_guard<std::mutex> lock(mutex_);
      return closed_;
    }

    void close() {
      std::lock_guard<std::mutex> lock(mutex_);
      closed_ = true;
      queue_.clear();
      callback_ = Callback{};
    }

   private:
    friend class TX;

    // Called from sender thread via TX object
    bool send(T &&message) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closed_) return false;
      queue_.push_back(std::move(message));
      post_event();
      return true;
    }

    bool send(const T &message) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closed_) return false;
      queue_.push_back(message);
      post_event();
      return true;
    }

    void post_event() {
      if (!posted_) {
        posted_ = true;
        io_->post([weak{this->weak_from_this()}]() {
          auto self = weak.lock();
          if (self) {
            self->on_receive();
          }
        });
      }
    }

    void on_receive() {
      boost::optional<T> msg;
      while ((msg = receive())) {
        callback_(std::move(msg.value()));
      }
    }

    std::mutex mutex_;
    std::deque<T> queue_;
    std::shared_ptr<boost::asio::io_context> io_;
    Callback callback_;
    bool posted_ = false;
    bool closed_ = false;
  };

}  // namespace fc::common

#endif  // CPP_FILECOIN_CORE_COMMON_RX_HPP
