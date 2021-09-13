# Storage market client

Module is implemented
according [spec](https://spec.filecoin.io/#section-systems.filecoin_markets.storage_market.storage-client).

Storage market client is a part of Fuhon node. It proposes storage deals to Storage market providers, tracks deal
statuses, and tracks what miner stores the data with the given CID.

## Storage deal flow

```
STORAGE_DEAL_UNKNOWN
 |
(ClientEventOpen) 
Proposes storage deal with deal parameters and adds peer to Discovery.
 |
STORAGE_DEAL_ENSURE_CLIENT_FUNDS
 |
(ClientEventFundingInitiated)
Asks funding message CID.
 |
STORAGE_DEAL_CLIENT_FUNDING
 |
(ClientEventFundsEnsured)
Gets and validates funding message CID.
 |
STORAGE_DEAL_FUNDS_ENSURED
 |
(ClientEventDealProposed)
Reads and verifies provider response, starts transferring.
If transfer type is manual, client asks for status in `pollWaiting()`.
If transfer type is graphsync the deal status changes after the data transfer is completed.
Since data is transferred, the provider must have accepted and published the deal proposal.
 |
STORAGE_DEAL_VALIDATING
 |
(ClientEventDealAccepted)
Verifies the deal is published
 |
STORAGE_DEAL_PROPOSAL_ACCEPTED
 |
(ClientEventDealPublished)
Waits for sector is committed. 
 |
STORAGE_DEAL_SEALING
 |
(ClientEventDealActivated)
Deal is on-chain, finalize deal and close streams.
 |
STORAGE_DEAL_ACTIVE
```
