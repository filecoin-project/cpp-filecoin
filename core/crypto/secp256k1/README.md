Reference implementation is 
https://github.com/ipsn/go-secp256k1/blob/master/secp256.go
(commit
43bfef0653c5a154ac1fff973f03b2664b394d3c).

It is a standalone clone of of `secp256k1` from https://github.com/ethereum/go-ethereum repository.

It uses as default:
 - SHA256 as digest algorithm
 - Uncompressed form of `PublicKey`
 - compact form of `Signature`
