/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_MESSAGE_HPP
#define CPP_FILECOIN_DATA_TRANSFER_MESSAGE_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "data_transfer/types.hpp"

namespace fc::data_transfer {

  /**
   * DataTransferRequest is a request message for the data transfer protocol
   */
  struct DataTransferRequest {
    std::string base_cid;
    bool is_cancel;
    std::vector<uint8_t> pid;
    bool is_part;
    bool is_pull;
    std::vector<uint8_t> selector;
    std::vector<uint8_t> voucher;
    std::string voucher_type;
    TransferId transfer_id;
  };

  /**
   * DataTransferResponse is a response message for the data transfer protocol
   */
  struct DataTransferResponse {
    bool is_accepted;
    TransferId transfer_id;
  };

  /**
   * DataTransferMessage is a message for the data transfer protocol (either
   * request or response) that can serialize to a protobuf
   */
  struct DataTransferMessage {
    bool is_request;
    boost::optional<DataTransferRequest> request;
    boost::optional<DataTransferResponse> response;
  };

  /**
   * Creates datatransfer request message
   * @param base_cid
   * @param is_pull
   * @param selector
   * @param voucher
   * @param voucher_type
   * @param transfer_id
   * @return request message
   */
  DataTransferMessage createRequest(std::string base_cid,
                                    bool is_pull,
                                    std::vector<uint8_t> selector,
                                    std::vector<uint8_t> voucher,
                                    std::string voucher_type,
                                    TransferId transfer_id);

  /**
   * Creates response data transfer message
   * @param is_accepted
   * @param transfer_id
   * @return response message
   */
  DataTransferMessage createResponse(bool is_accepted, TransferId transfer_id);

  CBOR_TUPLE(DataTransferRequest,
             base_cid,
             is_cancel,
             pid,
             is_part,
             is_pull,
             selector,
             voucher,
             voucher_type,
             transfer_id)

  CBOR_TUPLE(DataTransferResponse, is_accepted, transfer_id)

  CBOR_TUPLE(DataTransferMessage, is_request, request, response)

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_DATA_TRANSFER_MESSAGE_HPP
