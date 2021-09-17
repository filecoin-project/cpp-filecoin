/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/time.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(fc::clock, TimeFromStringError, e) {
  using fc::clock::TimeFromStringError;
  if (e == TimeFromStringError::kInvalidFormat) {
    return "Input has invalid format";
  }
  return "Unknown error";
}

namespace fc::clock {
  const static boost::posix_time::ptime kPtimeUnixZero(
      boost::gregorian::date(1970, 1, 1));

  std::string unixTimeToString(UnixTime time) {
    return boost::posix_time::to_iso_extended_string(
               kPtimeUnixZero + boost::posix_time::seconds{time.count()})
           + "Z";
  }

  outcome::result<UnixTime> unixTimeFromString(const std::string &str) {
    if (str.size() != 20 || str[str.size() - 1] != 'Z') {
      return TimeFromStringError::kInvalidFormat;
    }
    boost::posix_time::ptime ptime;
    try {
      ptime = boost::posix_time::from_iso_extended_string(
          str.substr(0, str.size() - 1));
    } catch (const boost::bad_lexical_cast &e) {
      return TimeFromStringError::kInvalidFormat;
    }
    return UnixTime{(ptime - kPtimeUnixZero).total_seconds()};
  }
}  // namespace fc::clock
