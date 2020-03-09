/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/clock/time.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(fc::clock, TimeFromStringError, e) {
  using fc::clock::TimeFromStringError;
  switch (e) {
    case TimeFromStringError::INVALID_FORMAT:
      return "Input has invalid format";
    default:
      return "Unknown error";
  }
}

namespace fc::clock {
  static boost::posix_time::ptime kPtimeUnixZero(boost::gregorian::date(1970,
                                                                        1,
                                                                        1));

  Time::Time(const UnixTimeNano &unix_time_nano)
      : unix_time_nano_(unix_time_nano) {}

  std::string Time::time() const {
    return boost::posix_time::to_iso_extended_string(
               kPtimeUnixZero
               + boost::posix_time::nanoseconds(unix_time_nano_.count()))
           + "Z";
  }

  UnixTime Time::unixTime() const {
    return std::chrono::duration_cast<UnixTime>(unix_time_nano_);
  }

  UnixTimeNano Time::unixTimeNano() const {
    return unix_time_nano_;
  }

  bool Time::operator<(const Time &other) const {
    return this->unix_time_nano_ < other.unix_time_nano_;
  }

  bool Time::operator==(const Time &other) const {
    return this->unix_time_nano_ == other.unix_time_nano_;
  }

  outcome::result<Time> Time::fromString(const std::string &str) {
    if (str.size() != 30 || str[str.size() - 1] != 'Z') {
      return TimeFromStringError::INVALID_FORMAT;
    }
    boost::posix_time::ptime ptime;
    try {
      ptime = boost::posix_time::from_iso_extended_string(str);
    } catch (const boost::bad_lexical_cast &e) {
      return TimeFromStringError::INVALID_FORMAT;
    }
    return Time(UnixTimeNano((ptime - kPtimeUnixZero).total_nanoseconds()));
  }
}  // namespace fc::clock
