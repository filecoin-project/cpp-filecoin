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
  HttpUri::HttpUri()
      : _scheme(Scheme::UNDEFINED),
        _port(0),
        _hasQuery(false),
        _hasFragment(false) {}

  HttpUri::HttpUri(const std::string &uri)
      : _scheme(Scheme::UNDEFINED),
        _port(0),
        _hasQuery(false),
        _hasFragment(false) {
    try {
      parse(uri.data(), uri.length());
    } catch (const std::exception &exception) {
      throw std::runtime_error(std::string("Bad URI ← ") + exception.what());
    }
  }

  void HttpUri::parse(const char *string, size_t length) {
    auto s = string;
    auto end = string + length;

    // scheme:
    if (length >= 2 && strncmp(s, "//", 2) == 0) {
      _scheme = Scheme::UNDEFINED;
      s += 2;
    } else if (*s == '/') {
      _scheme = Scheme::UNDEFINED;
      goto path;
    } else if (length >= 7 && strncasecmp(s, "http://", 7) == 0) {
      _scheme = Scheme::HTTP;
      _port = 80;
      s += 7;
    } else if (length >= 8 && strncasecmp(s, "https://", 8) == 0) {
      _scheme = Scheme::HTTPS;
      _port = 443;
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
      _host.push_back(static_cast<char>(tolower(*s)));
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
      _port = static_cast<uint16_t>(port);
    }

  // path:
  path:
    if (*s == '/') {
      while (*s != 0 && *s != '?' && *s != '#' && isspace(*s) == 0) {
        _path.push_back(*s);
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
      _hasQuery = true;
      while (*s != 0 && *s != '#' && isspace(*s) == 0) {
        _query.push_back(*s);
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
      _hasFragment = true;
      while (*s != 0 && isspace(*s) == 0) {
        _fragment.push_back(*s);
        if (++s > end) {
          throw std::runtime_error("Out of data");
        }
      }
    }
  }

  const std::string &HttpUri::str() const {
    std::stringstream ss;

    if (_scheme == Scheme::HTTP) ss << "http://";
    if (_scheme == Scheme::HTTPS) ss << "https://";
    if (_scheme == Scheme::UNDEFINED && !_host.empty()) ss << "//";

    ss << _host;

    if (_port != 0
        && ((_scheme == Scheme::HTTP && _port != 80)
            || (_scheme == Scheme::HTTPS && _port != 443))) {
      ss << ':' << _port;
    }

    ss << _path;

    if (_hasQuery) {
      ss << '?' << _query;
    }

    if (_hasFragment) {
      ss << '#' << _fragment;
    }

    _thisAsString = std::move(ss.str());

    return _thisAsString;
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
