/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <future>

#include "adt/channel.hpp"
#include "common/error_text.hpp"
#include "common/outcome.hpp"
#include "primitives/jwt/jwt.hpp"

#define API_METHOD(name, perm, result, ...) \
  ApiMethod<perm, result, ##__VA_ARGS__> name{"Filecoin." #name};

namespace fc::api {
  using adt::Channel;
  using primitives::jwt::Permission;

  template <typename... T>
  using ParamsTuple =
      std::tuple<std::remove_const_t<std::remove_reference_t<T>>...>;

  template <const Permission &perm, typename T, typename... Ts>
  class ApiMethod {
   public:
    using Result = T;
    using OutcomeResult = outcome::result<T>;
    using Params = ParamsTuple<Ts...>;
    using Callback = std::function<void(OutcomeResult)>;
    using FunctionSimpleSignature = OutcomeResult(Ts...);
    using FunctionSignature = void(Callback, Ts...);

    OutcomeResult operator()(Ts... args) const {
      if (!f_) {
        return ERROR_TEXT("API not set up");
      }
      std::promise<OutcomeResult> wait;
      f_([&wait](const OutcomeResult &res) { return wait.set_value(res); },
         args...);
      return wait.get_future().get();
    }
    void operator()(Callback cb, Ts... args) const {
      if (!f_) {
        return cb(ERROR_TEXT("API not set up"));
      }
      f_(cb, args...);
    }

    explicit ApiMethod(std::string name) noexcept : name_(std::move(name)){};

    ApiMethod &operator=(std::function<FunctionSimpleSignature> &&f) {
      if (f) {
        f_ = [wf{std::move(f)}](auto &&cb, auto &&... args) -> void {
          cb(wf(std::forward<decltype(args)>(args)...));
        };
      } else {
        f_ = {};
      }
      return *this;
    }

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

    [[nodiscard]] std::string getPerm() const {
      return perm;
    }

   private:
    std::string name_;

    std::function<FunctionSignature> f_;
  };

  template <typename T>
  struct Chan {
    using Type = T;
    Chan() = default;
    // TODO (a.chernyshov) (FIL-414) Rework class to make constructor explicit
    // or remove comment and close the task
    // NOLINTNEXTLINE(google-explicit-constructor)
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
}  // namespace fc::api
