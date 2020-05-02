/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ADT_CHANNEL_HPP
#define CPP_FILECOIN_ADT_CHANNEL_HPP

#include <mutex>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "common/visitor.hpp"
#include "common/which.hpp"

namespace fc::adt {
  using common::which;

  template <typename T>
  class Channel {
   public:
    using Queue = std::pair<std::vector<T>, bool>;
    using Handler = std::function<bool(boost::optional<T>)>;
    struct Closed {};

    Channel() = default;
    Channel(Channel &&) = default;
    ~Channel() {
      closeRead();
    }

    bool canWrite() {
      std::lock_guard lock{mutex};
      return visit_in_place(
          state,
          [](const Queue &q) { return !q.second; },
          [](const Handler &) { return true; },
          [](const Closed &) { return false; });
    }

    bool write(T value) {
      std::lock_guard lock{mutex};
      if (which<Queue>(state)) {
        auto &[values, closed] = boost::get<Queue>(state);
        if (closed) {
          return false;
        }
        values.push_back(std::move(value));
      } else if (which<Handler>(state)) {
        if (!boost::get<Handler>(state)(std::move(value))) {
          state = Closed{};
        }
      } else {
        return false;
      }
      return true;
    }

    bool closeWrite() {
      std::lock_guard lock{mutex};
      if (which<Queue>(state)) {
        auto &closed = boost::get<Queue>(state).second;
        if (closed) {
          return false;
        }
        closed = true;
      } else if (which<Handler>(state)) {
        boost::get<Handler>(state)({});
        state = Closed{};
      } else {
        return false;
      }
      return true;
    }

    bool read(Handler handler) {
      std::lock_guard lock{mutex};
      if (!which<Queue>(state)) {
        return false;
      }
      auto &[values, closed] = boost::get<Queue>(state);
      auto stop = false;
      for (auto &&value : values) {
        stop = !handler(std::move(value));
        if (stop) {
          break;
        }
      }
      if (closed || stop) {
        if (!stop) {
          handler({});
        }
        state = Closed{};
      } else {
        state = handler;
      }
      return true;
    }

    bool closeRead() {
      std::lock_guard lock{mutex};
      if (which<Closed>(state)) {
        return false;
      }
      if (which<Handler>(state)) {
        boost::get<Handler>(state)({});
      }
      state = Closed{};
      return true;
    }

   private:
    boost::variant<Queue, Handler, Closed> state{Queue{{}, false}};
    std::mutex mutex;
  };

  template <typename T>
  void writeMany(std::vector<std::shared_ptr<Channel<T>>> &cs, T v) {
    cs.erase(std::remove_if(
                 cs.begin(), cs.end(), [&](auto &c) { return !c->write(v); }),
             cs.end());
  }
}  // namespace fc::adt

#endif  // CPP_FILECOIN_ADT_CHANNEL_HPP
