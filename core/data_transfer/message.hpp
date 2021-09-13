/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "data_transfer/types.hpp"

namespace fc::data_transfer {
  enum class MessageType {
    kNewMessage,
    kUpdateMessage,
    kCancelMessage,
    kCompleteMessage,
    kVoucherMessage,
    kVoucherResultMessage,
  };

  /**
   * DataTransferRequest is a request message for the data transfer protocol
   */
  struct DataTransferRequest {
    boost::optional<CID> base_cid;
    MessageType type{};
    bool is_pause{};
    bool is_part{};
    bool is_pull{};
    boost::optional<Selector> selector;
    boost::optional<CborRaw> voucher;
    std::string voucher_type;
    TransferId transfer_id{};

    inline bool operator==(const DataTransferRequest &other) const {
      return base_cid == other.base_cid && type == other.type
             && is_pause == other.is_pause && is_part == other.is_part
             && is_pull == other.is_pull && selector == other.selector
             && voucher == other.voucher && voucher_type == other.voucher_type
             && transfer_id == other.transfer_id;
    }
  };

  /**
   * DataTransferResponse is a response message for the data transfer protocol
   */
  struct DataTransferResponse {
    MessageType type{};
    bool is_accepted{};
    bool is_pause{};
    TransferId transfer_id{};
    boost::optional<CborRaw> voucher;
    std::string voucher_type;

    inline bool operator==(const DataTransferResponse &other) const {
      return type == other.type && is_accepted == other.is_accepted
             && is_pause == other.is_pause && transfer_id == other.transfer_id
             && voucher == other.voucher && voucher_type == other.voucher_type;
    }
  };

  struct DataTransferMessage {
    DataTransferMessage() = default;
    DataTransferMessage(DataTransferRequest request)
        : is_request{true}, request{std::move(request)} {}
    DataTransferMessage(DataTransferResponse response)
        : is_request{false}, response{std::move(response)} {}

    bool is_request{};
    boost::optional<DataTransferRequest> request;
    boost::optional<DataTransferResponse> response;

    inline bool operator==(const DataTransferMessage &other) const {
      return is_request == other.is_request && request == other.request
             && response == other.response;
    }

    inline auto dtid() const {
      return is_request ? request->transfer_id : response->transfer_id;
    }
  };

  CBOR_TUPLE(DataTransferRequest,
             base_cid,
             type,
             is_pause,
             is_part,
             is_pull,
             selector,
             voucher,
             voucher_type,
             transfer_id)

  CBOR_TUPLE(DataTransferResponse,
             type,
             is_accepted,
             is_pause,
             transfer_id,
             voucher,
             voucher_type)

  CBOR_TUPLE(DataTransferMessage, is_request, request, response)

}  // namespace fc::data_transfer
