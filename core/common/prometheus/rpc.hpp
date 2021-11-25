/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/rpc/rpc.hpp"
#include "common/prometheus/metrics.hpp"
#include "common/prometheus/since.hpp"

namespace fc::api::rpc {
  inline auto &metricApiTime() {
    static auto &x{prometheus::BuildHistogram()
                       .Name("lotus_api_request_duration_ms")
                       .Help("Duration of API requests")
                       .Register(prometheusRegistry())};
    return x;
  }

  inline Method metricApiTime(std::string name, Method f) {
    return [name{std::move(name)}, f{std::move(f)}](
               const Value &value,
               Respond respond,
               MakeChan make_chan,
               Send send,
               const Permissions &permissions) {
      f(
          value,
          [name{std::move(name)}, respond{std::move(respond)}, since{Since{}}](
              auto &&value) {
            const auto time{since.ms()};
            metricApiTime()
                .Add({{"endpoint", name}}, kDefaultPrometheusMsBuckets)
                .Observe(time);
            respond(std::move(value));
          },
          std::move(make_chan),
          std::move(send),
          permissions);
    };
  }

  inline void metricApiTime(Rpc &rpc) {
    for (auto &[name, value] : rpc.ms) {
      value = metricApiTime(name, value);
    }
  }
}  // namespace fc::api::rpc
