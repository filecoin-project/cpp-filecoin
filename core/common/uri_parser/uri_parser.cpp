/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

// Copyright © 2017-2019 Dmitriy Khaustov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Dmitriy Khaustov aka xDimon
// Contacts: khaustov.dm@gmail.com
// File created on: 2017.03.26

// HttpUri.cpp

#include "common/uri_parser/uri_parser.hpp"
#include "common/uri_parser/percent_encoding.hpp"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace fc::common {
  HttpUri::HttpUri(const std::string &uri) {
    try {
      parse(uri);
    } catch (const std::exception &exception) {
      throw std::runtime_error(std::string("Bad URI ← ") + exception.what());
    }
  }

  void HttpUri::parse(const std::string &string) {
    const char *s = string.c_str();
    const auto *end = string.c_str() + string.size();

    // scheme:
    if (string.length() >= 2 && strncmp(s, "//", 2) == 0) {
      scheme_ = Scheme::UNDEFINED;
      s += 2;
    } else if (*s == '/') {
      scheme_ = Scheme::UNDEFINED;
      parsePath(s, end);
    } else if (string.length() >= 7 && strncasecmp(s, "http://", 7) == 0) {
      scheme_ = Scheme::HTTP;
      port_ = 80;
      s += 7;
    } else if (string.length() >= 8 && strncasecmp(s, "https://", 8) == 0) {
      scheme_ = Scheme::HTTPS;
      port_ = 443;
      s += 8;
    } else {
      throw std::runtime_error("Wrong scheme");
    }

    // host:
    while (*s != 0 && *s != ':' && *s != '/' && *s != '?' && *s != '#'
           && isspace(*s) == 0) {
      if (isalnum(*s) == 0 && *s != '.' && *s != '-') {
        throw std::runtime_error("Wrong hostname");
      }
      host_.push_back(static_cast<char>(tolower(*s)));
      if (++s > end) {
        throw std::runtime_error("Out of data");
      }
    }

    // port:
    if (*s == ':') {
      if (++s > end) {
        throw std::runtime_error("Out of data");
      }
      uint64_t port = 0;
      while (*s != 0 && *s != '/' && *s != '?' && *s != '#'
             && isspace(*s) == 0) {
        if (isdigit(*s) == 0) {
          throw std::runtime_error("Wrong port");
        }
        port = port * 10 + *s - '0';
        if (port < 1 || port > 65535) {
          throw std::runtime_error("Wrong port");
        }
        if (++s > end) {
          throw std::runtime_error("Out of data");
        }
      }
      port_ = static_cast<uint16_t>(port);
    }

    // path:
    parsePath(s, end);
  }

  void HttpUri::parsePath(const char *s, const char *end) {
    if (*s == '/') {
      while (*s != 0 && *s != '?' && *s != '#' && isspace(*s) == 0) {
        path_.push_back(*s);
        if (++s > end) {
          throw std::runtime_error("Out of data");
        }
      }
    }

    // query:
    if (*s == '?') {
      if (++s > end) {
        throw std::runtime_error("Out of data");
      }
      hasQuery_ = true;
      while (*s != 0 && *s != '#' && isspace(*s) == 0) {
        query_.push_back(*s);
        if (++s > end) {
          throw std::runtime_error("Out of data");
        }
      }
    }

    // fragment:
    if (*s == '#') {
      if (++s > end) {
        throw std::runtime_error("Out of data");
      }
      hasFragment_ = true;
      while (*s != 0 && isspace(*s) == 0) {
        fragment_.push_back(*s);
        if (++s > end) {
          throw std::runtime_error("Out of data");
        }
      }
    }
  }

  const std::string &HttpUri::str() const {
    std::stringstream ss;

    if (scheme_ == Scheme::HTTP) ss << "http://";
    if (scheme_ == Scheme::HTTPS) ss << "https://";
    if (scheme_ == Scheme::UNDEFINED && !host_.empty()) ss << "//";

    ss << host_;

    if (port_ != 0
        && ((scheme_ == Scheme::HTTP && port_ != 80)
            || (scheme_ == Scheme::HTTPS && port_ != 443))) {
      ss << ':' << port_;
    }

    ss << path_;

    if (hasQuery_) {
      ss << '?' << query_;
    }

    if (hasFragment_) {
      ss << '#' << fragment_;
    }

    thisAsString_ = ss.str();

    return thisAsString_;
  }

  std::string HttpUri::urldecode(const std::string &input_) {
    std::string input(input_);
    std::replace(input.begin(), input.end(), '+', ' ');
    return PercentEncoding::decode(input);
  }

  std::string HttpUri::urlencode(const std::string &input) {
    return PercentEncoding::encode(input);
  }
}  // namespace fc::common
