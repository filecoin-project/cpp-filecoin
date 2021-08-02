/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <future>

#include "adt/channel.hpp"
#include "common/error_text.hpp"
#include "common/outcome.hpp"

#define API_METHOD(_name, _result, ...)                                    \
  struct _##_name : std::function<outcome::result<_result>(__VA_ARGS__)> { \
    using function::function;                                              \
    using Result = _result;                                                \
    using Params = ParamsTuple<__VA_ARGS__>;                               \
    using FunctionSignature = outcome::result<_result>(__VA_ARGS__);       \
    static constexpr auto name = "Filecoin." #_name;                       \
  } _name;

namespace fc::api {
  using adt::Channel;

  template <typename... T>
  using ParamsTuple =
      std::tuple<std::remove_const_t<std::remove_reference_t<T>>...>;

  template <typename T>
  struct Chan {
    using Type = T;
    Chan() = default;
    Chan(std::shared_ptr<Channel<T>> channel) : channel{std::move(channel)} {}
    static Chan make() {
      return std::make_shared<Channel<T>>();
    }
    uint64_t id{};
    std::shared_ptr<Channel<T>> channel;
  };

  template <typename T>
  struct is_chan : std::false_type {};

  template <typename T>
  struct is_chan<Chan<T>> : std::true_type {};

  template <typename T>
  struct Wait {
    using Type = T;
    using Result = outcome::result<T>;
    using Cb = std::function<void(Result)>;

    Wait() = default;
    Wait(std::shared_ptr<Channel<Result>> channel)
        : channel{std::move(channel)} {}
    static Wait make() {
      return std::make_shared<Channel<Result>>();
    }

    void waitOwn(Cb cb) {
      wait([c{channel}, cb{std::move(cb)}](auto &&v) { cb(v); });
    }

    void wait(Cb cb) {
      channel->read([cb{std::move(cb)}](auto opt) {
        if (opt) {
          cb(std::move(*opt));
        } else {
          cb(ERROR_TEXT("Wait::wait: channel closed"));
        }
        return false;
      });
    }

    auto waitSync() {
      std::promise<Result> p;
      wait([&](auto v) { p.set_value(std::move(v)); });
      return p.get_future().get();
    }

    std::shared_ptr<Channel<Result>> channel;
  };

  template <typename T, typename F>
  auto waitCb(F &&f) {
    return [f{std::forward<F>(f)}](auto &&...args) {
      auto channel{std::make_shared<Channel<outcome::result<T>>>()};
      f(std::forward<decltype(args)>(args)..., [channel](auto &&_r) {
        channel->write(std::forward<decltype(_r)>(_r));
      });
      return Wait{channel};
    };
  }

  template <typename T>
  struct is_wait : std::false_type {};

  template <typename T>
  struct is_wait<Wait<T>> : std::true_type {};
}  // namespace fc::api
