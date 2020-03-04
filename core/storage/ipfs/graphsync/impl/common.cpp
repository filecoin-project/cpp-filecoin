/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs::graphsync, Error, e) {
  using E = fc::storage::ipfs::graphsync::Error;
  switch (e) {
    case E::MESSAGE_PARSE_ERROR:
      return "message parse error";
    case E::MESSAGE_SIZE_OUT_OF_BOUNDS:
      return "message size out of bounds";
    case E::MESSAGE_VALIDATION_FAILED:
      return "message validation failed";
    case E::MESSAGE_SERIALIZE_ERROR:
      return "message serialize error";
    case E::STREAM_NOT_READABLE:
      return "stream is not readable";
    case E::MESSAGE_READ_ERROR:
      return "message read error";
    case E::STREAM_NOT_WRITABLE:
      return "stream is not writable";
    case E::WRITE_QUEUE_OVERFLOW:
      return "write queue overflow";
    case E::MESSAGE_WRITE_ERROR:
      return "message write error";
    default:
      break;
  }
  return "unknown error";
}

namespace fc::storage::ipfs::graphsync {

  common::Logger logger() {
    static common::Logger graphsync_logger = common::createLogger("graphsync");
    return graphsync_logger;
  }

  // TODO(artem)
  // ResponseStatusCode errorToStatusCode(outcome::result<void> error) {
  //  error.error().category();
  //}

  bool isTerminal(ResponseStatusCode code) {
    return code < 10 || code >= 20;
  }

  std::string statusCodeToString(ResponseStatusCode code) {
    switch (code) {
// clang-format off
#define CHECK_CASE(X) case RS_##X: return #X;
      CHECK_CASE(NO_PEERS)
      CHECK_CASE(CANNOT_CONNECT)
      CHECK_CASE(TIMEOUT)
      CHECK_CASE(CONNECTION_ERROR)
      CHECK_CASE(INTERNAL_ERROR)
      CHECK_CASE(REJECTED_LOCALLY)
      CHECK_CASE(REQUEST_ACKNOWLEDGED)
      CHECK_CASE(ADDITIONAL_PEERS)
      CHECK_CASE(NOT_ENOUGH_GAS)
      CHECK_CASE(OTHER_PROTOCOL)
      CHECK_CASE(PARTIAL_RESPONSE)
      CHECK_CASE(FULL_CONTENT)
      CHECK_CASE(PARTIAL_CONTENT)
      CHECK_CASE(REJECTED)
      CHECK_CASE(TRY_AGAIN)
      CHECK_CASE(REQUEST_FAILED)
      CHECK_CASE(LEGAL_ISSUES)
      CHECK_CASE(NOT_FOUND)
#undef CHECK_CASE
        // clang-format on

      default:
        break;
    }
    return "UNKNOWN";
  }

}  // namespace fc::storage::ipfs::graphsync
