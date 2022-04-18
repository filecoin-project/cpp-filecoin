#pragma once

#include "common/prometheus/metrics.hpp"
#include "common/prometheus/since.hpp"

#define FVM_METRIC(name)                                                \
  static auto &fvm_metric_##name{                                       \
      fvmMetric().Add({{"name", #name}}, kDefaultPrometheusMsBuckets)}; \
  const auto BOOST_OUTCOME_TRY_UNIQUE_NAME{gsl::finally(                \
      [since{Since{}}] { fvm_metric_##name.Observe(since.ms()); })};

namespace fc::vm::fvm {
  inline auto &fvmMetric() {
    static auto &x{prometheus::BuildHistogram().Name("fvm").Register(
        prometheusRegistry())};
    return x;
  }
}  // namespace fc::vm::fvm
