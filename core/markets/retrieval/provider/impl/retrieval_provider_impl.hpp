/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_IMPL_HPP

#include "api/api.hpp"
#include "common/libp2p/cbor_stream.hpp"
#include "common/logger.hpp"
#include "data_transfer/dt.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"
#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "markets/retrieval/provider/retrieval_provider.hpp"
#include "miner/miner.hpp"
#include "storage/ipld/traverser.hpp"
#include "storage/piece/piece_storage.hpp"

namespace fc::markets::retrieval::provider {
  using common::libp2p::CborStream;
  using data_transfer::DataTransfer;
  using data_transfer::PeerDtId;
  using data_transfer::PeerGsId;
  using ::fc::miner::Miner;
  using ::fc::sector_storage::Manager;
  using ::fc::storage::ipld::traverser::Traverser;
  using ::fc::storage::piece::PieceInfo;
  using ::fc::storage::piece::PieceStorage;
  using libp2p::Host;
  using primitives::BigInt;
  using primitives::SectorNumber;
  using primitives::piece::PieceData;
  using primitives::piece::UnpaddedPieceSize;
  using GsResStatus = ::fc::storage::ipfs::graphsync::ResponseStatusCode;

  /**
   * @struct Provider config
   */
  struct ProviderConfig {
    TokenAmount price_per_byte;
    uint64_t payment_interval;
    uint64_t interval_increase;
    TokenAmount unseal_price;
  };

  struct DealState {
    DealState(std::shared_ptr<Ipld> ipld,
              const PeerDtId &pdtid,
              const PeerGsId &pgsid,
              const DealProposal &proposal)
        : proposal{proposal},
          state{proposal.params},
          pdtid{pdtid},
          pgsid{pgsid},
          traverser{*ipld, proposal.payload_cid, proposal.params.selector} {}

    DealProposal proposal;
    State state;
    PeerDtId pdtid;
    PeerGsId pgsid;
    bool unsealed{false};
    Traverser traverser;
  };

  class RetrievalProviderImpl
      : public RetrievalProvider,
        public std::enable_shared_from_this<RetrievalProviderImpl> {
   public:
    RetrievalProviderImpl(std::shared_ptr<Host> host,
                          std::shared_ptr<DataTransfer> datatransfer,
                          std::shared_ptr<api::Api> api,
                          std::shared_ptr<PieceStorage> piece_storage,
                          IpldPtr ipld,
                          const ProviderConfig &config,
                          std::shared_ptr<Manager> sealer,
                          std::shared_ptr<Miner> miner);

    void onProposal(const PeerDtId &pdtid,
                    const PeerGsId &pgsid,
                    const DealProposal &proposal);
    void onPayment(std::shared_ptr<DealState> deal, const DealPayment &payment);
    void doUnseal(std::shared_ptr<DealState> deal);
    void doBlocks(std::shared_ptr<DealState> deal);
    void doComplete(std::shared_ptr<DealState> deal);
    bool hasOwed(std::shared_ptr<DealState> deal);
    void doFail(std::shared_ptr<DealState> deal, std::string error);

    void start() override;

    void setPricePerByte(TokenAmount amount) override;

    void setPaymentInterval(uint64_t payment_interval,
                            uint64_t payment_interval_increase) override;

   private:
    /**
     * Handle query stream
     * @param stream - cbor stream
     */
    void handleQuery(const std::shared_ptr<CborStream> &stream);

    /**
     * Look for piece by cid and make response
     * @param query with piece look for
     * @return network response
     */
    outcome::result<QueryResponse> makeQueryResponse(const QueryRequest &query);

    /**
     * Send error response for query and close stream
     * @param stream to send and close
     * @param message error to send
     */
    void respondErrorQueryResponse(const std::shared_ptr<CborStream> &stream,
                                   const std::string &message);

    /**
     * Unseal sector
     * @param sector_id - id of sector
     * @param offset - offset of piece
     * @param size - size of piece
     * @return Read PieceData
     */
    outcome::result<PieceData> unsealSector(SectorNumber sector_id,
                                            UnpaddedPieceSize offset,
                                            UnpaddedPieceSize size);

    std::shared_ptr<Host> host_;
    std::shared_ptr<DataTransfer> datatransfer_;
    std::shared_ptr<api::Api> api_;
    std::shared_ptr<PieceStorage> piece_storage_;
    std::shared_ptr<Ipld> ipld_;
    ProviderConfig config_;
    std::shared_ptr<Manager> sealer_;
    std::shared_ptr<Miner> miner_;
    common::Logger logger_ = common::createLogger("RetrievalProvider");
  };
}  // namespace fc::markets::retrieval::provider

#endif
