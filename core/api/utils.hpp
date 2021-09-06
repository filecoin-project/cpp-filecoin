/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <future>

#include "adt/channel.hpp"
#include "common/error_text.hpp"
#include "common/outcome.hpp"

#define API_METHOD(_name, _result, ...) \
  ApiMethod<_result, ##__VA_ARGS__> _name{"Filecoin." #_name};

namespace fc::api {
  using adt::Channel;

  template <typename... T>
  using ParamsTuple =
      std::tuple<std::remove_const_t<std::remove_reference_t<T>>...>;

  template <typename T, typename... Ts>
  class ApiMethod {
   public:
    using Result = T;
    using OutcomeResult = outcome::result<T>;
    using Params = ParamsTuple<Ts...>;
    using Callback = std::function<void(OutcomeResult)>;
    using FunctionSignature = void(Callback, Ts...);

    OutcomeResult operator()(Ts... args) const {
      // TODO: CHECK F_
      std::promise<OutcomeResult> wait;
      f_([&wait](const OutcomeResult &res) { return wait.set_value(res); },
         args...);
      return wait.get_future().get();
    }
    void operator()(const std::function<void(OutcomeResult)> &cb,
                    Ts... args) const {
      // TODO: CHECK F_
      f_(cb, args...);
    }

    explicit ApiMethod(std::string name) noexcept : name_(std::move(name)){};

    ApiMethod &operator=(std::function<FunctionSignature> &&f) {
      f_ = std::forward<std::function<FunctionSignature>>(f);
      return *this;
    }

    explicit operator bool() const {
      return static_cast<bool>(f_);
    }

    [[nodiscard]] std::string getName() const {
      return name_;
    }

   private:
    std::string name_;

    std::function<FunctionSignature> f_;
  };

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

  template <typename F>
  auto wrapCb(F &&f) {
    return [f{std::forward<F>(f)}](auto &&cb, auto &&... args) -> void {
      cb(f(std::forward<decltype(args)>(args)...));
    };
  }
}  // namespace fc::api
