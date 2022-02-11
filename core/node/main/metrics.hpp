/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <prometheus/text_serializer.h>
#include <libp2p/common/metrics/instance_count.hpp>
#include <sstream>

#include "clock/chain_epoch_clock.hpp"
#include "clock/utc_clock.hpp"
#include "common/fd_usage.hpp"
#include "common/memory_usage.hpp"
#include "common/prometheus/metrics.hpp"
#include "node/events.hpp"
#include "node/main/builder.hpp"
#include "node/sync_job.hpp"

namespace fc::node {
  struct Metrics {
    using Clock = std::chrono::steady_clock;

    Metrics(const NodeObjects &o, Clock::time_point start_time)
        : o{o}, start_time{start_time} {
      _possible_head = o.events->subscribePossibleHead(
          [this](auto &e) { height_known = e.height; });
    }

    std::string prometheus() const {
      auto families{prometheusRegistry().Collect()};
      using ::prometheus::ClientMetric;
      using ::prometheus::MetricType;
      auto manual{[&](MetricType type,
                      std::string name,
                      std::string help) -> ClientMetric & {
        auto family{
            std::find_if(families.begin(), families.end(), [&](auto &family) {
              return family.name == name;
            })};
        if (family == families.end()) {
          family = families.emplace(families.end());
          family->name = std::move(name);
          family->help = std::move(help);
        }
        family->type = type;
        return family->metric.emplace_back();
      }};
      auto metric{[&](std::string name, double value) -> ClientMetric & {
        auto &metric{manual(MetricType::Gauge, std::move(name), "")};
        metric.gauge.value = value;
        return metric;
      }};

      metric("uptime",
             std::chrono::duration_cast<std::chrono::seconds>(Clock::now()
                                                              - start_time)
                 .count());

      auto [vm_size, vm_rss]{memoryUsage()};
      metric("vm_size", vm_size);
      metric("vm_rss", vm_rss);

      metric("fd", fdUsage());

      std::shared_lock ts_lock{*o.env_context.ts_branches_mutex};
      metric("ts_branches", o.ts_branches->size());
      auto height_head{o.ts_main->chain.rbegin()->first};
      ts_lock.unlock();

      metric("height_head", height_head);
      metric("height_attached",
             std::max(height_head, o.sync_job->metricAttachedHeight()));
      metric("height_known", std::max(height_head, height_known.load()));
      const auto height_expected{
          o.chain_epoch_clock->epochAtTime(o.utc_clock->nowUTC()).value()};
      metric("height_expected", height_expected);

      auto car{[&](auto _size, auto _count, auto _tmp, auto &ipld) {
        if (ipld) {
          std::shared_lock index_lock{ipld->index_mutex};
          std::shared_lock written_lock{ipld->written_mutex};
          metric(_size, ipld->car_offset);
          metric(_count, ipld->index->size());
          metric(_tmp, ipld->written.size());
        }
      }};
      {
        std::unique_lock ipld_lock{o.compacter->ipld_mutex};
        car("car_size", "car_count", "car_tmp", o.compacter->old_ipld);
        car("car2_size", "car2_count", "car2_tmp", o.compacter->new_ipld);
      }

      auto &instances{libp2p::metrics::instance::State::get()};
      std::unique_lock instances_lock{instances.mutex};
      for (auto &[type, count] : instances.counts) {
        metric("instances", count)
            .label.emplace_back(ClientMetric::Label{"type", std::string{type}});
      }
      instances_lock.unlock();

      manual(MetricType::Gauge,
             "lotus_chain_node_height",
             "Current Height of the node")
          .gauge.value = height_head;
      manual(MetricType::Gauge,
             "lotus_chain_node_height_expected",
             "Expected Height of the node")
          .gauge.value = height_expected;
      manual(MetricType::Gauge,
             "lotus_chain_node_worker_height",
             "Height of workers on the node")
          .gauge.value = height_head;

      return ::prometheus::TextSerializer{}.Serialize(families);
    }

    const NodeObjects &o;
    Clock::time_point start_time;

    std::atomic_int64_t height_known{};
    sync::events::Connection _possible_head;
  };
}  // namespace fc::node
