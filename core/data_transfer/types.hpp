/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_TYPES_HPP
#define CPP_FILECOIN_DATA_TRANSFER_TYPES_HPP

#include <libp2p/peer/peer_info.hpp>
#include "clock/time.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"
#include "storage/ipld/ipld_node.hpp"
#include "storage/ipld/selector.hpp"

namespace fc::data_transfer {

  using fc::storage::ipfs::graphsync::statusCodeToString;
  using libp2p::peer::PeerInfo;
  using storage::ipld::IPLDNode;
  using storage::ipld::Selector;

  /**
   * TransferID is an identifier for a data transfer, shared between
   * request/responder and unique to the requester
   */
  using TransferId = uint64_t;

  /**
   * Voucher is used to validate a data transfer request against the  underlying
   * storage or retrieval deal that precipitated it. The only requirement is a
   * voucher can read and write from bytes, and has a string identifier type
   */
  struct Voucher {
    std::string type;
    std::vector<uint8_t> bytes;
  };

  /**
   * Status is the status of transfer for a given channel
   */
  enum class Status : int {
    /** The data transfer is in progress */
    ONGOING = 1,
    /** The data transfer is completed successfully */
    COMPLETED,
    /** The data transfer failed */
    FAILED,
    /** The searched for data transfer does not exist */
    CHANNEL_NOT_FOUND_ERROR
  };

  /**
   * ChannelID is a unique identifier for a channel, distinct by both the other
   * party's peer ID + the transfer ID
   */
  struct ChannelId {
    PeerInfo initiator;
    TransferId id;

    bool operator<(const ChannelId &other) const {
      return id < other.id
             || (id == other.id
                 && peerInfoToPrettyString(initiator)
                        < peerInfoToPrettyString(other.initiator));
    }
  };

  /**
   * Channel represents all immutable parameters for a single data transfer
   */
  struct Channel {
    /**
     * an identifier for this channel shared by request and responder, set by
     * requester through protocol
     */
    TransferId transfer_id;

    /** base CID for the piece being transferred */
    CID base_cid;

    /** portion of Piece to return, specified by an IPLD selector */
    std::shared_ptr<Selector> selector;

    /** used to verify this channel */
    Buffer voucher;

    /** the party that is sending the data (not who initiated the request) */
    PeerInfo sender;

    /** the party that is receiving the data (not who initiated the request) */
    PeerInfo recipient;

    /** expected amount of data to be transferred */
    size_t total_size;
  };

  /** ChannelState is immutable channel data plus mutable state */
  struct ChannelState {
    Channel channel;

    /** total bytes sent from this node (0 if receiver) */
    size_t sent{0};

    /** total bytes received by this node (0 if sender) */
    size_t received{0};
  };

  /**
   * EventCode is a name for an event that occurs on a data transfer channel
   */
  enum class EventCode : int {
    /** Open is an event occurs when a channel is first opened */
    OPEN = 1,

    /**
     * Progress is an event that gets emitted every time more data is
     * transferred
     */
    PROGRESS,

    /** Error is an event that emits when an error occurs in a data transfer */
    ERROR,

    /** Complete is emitted when a data transfer is complete */
    COMPLETE
  };

  /**
   * Event is a struct containing information about a data transfer event
   */
  struct Event {
    EventCode code;         // What type of event it is
    std::string message;    // Any clarifying information about the event
    clock::Time timestamp;  // when the event happened
  };

  /**
   * Subscriber is a callback that is called when events are emitted
   */
  class Subscriber {
   public:
    virtual ~Subscriber() = default;

    virtual void notify(const Event &event,
                        const ChannelState &channel_state) = 0;
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_DATA_TRANSFER_TYPES_HPP
