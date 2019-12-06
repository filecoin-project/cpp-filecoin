# LEB128 codec C++ implementation

Encoding and decoding the following data types:
* `uint8_t, uint16_t, uint32_t, uint64_t`
* `uint128_t, uint256_t, uint512_t, uint1024_t` (boost::multiprecision) 

## LEB128EncodeStream
Class LEB128EncodeStream is in charge of encoding data
```c
#include "codec/leb128/leb128_encode_stream.hpp"
using fc::codec::leb128::LEB128EncodeStream;

LEB128EncodeStream s;
uint64_t data = 481516u;
s << data;
std::vector<uint8_t> encoded = s.data();
```

## LEB128DecodeStream
Class LEB128 is in charge of decoding data
```c
#include "codec/leb128/leb128_decode_stream.hpp"
using fc::codec::leb128::LEB128DecodeStream;

std::vector<uint8_t> encoded{0x88, 0xA1, 0x02};
LEB128DecodeStream s{ encoded };
uint16_t data;
try {
  s >> data;
} catch (std::exception &) {
  // handle error
}
```

## Convenience functions
Wrappers around encode/decode streams

```c
#include "codec/leb128/leb128.hpp"
using fc::codec::leb128::encode;
using fc::codec::leb128::decode;

T data;
std::vector<uint8_t> encoded = encode(data);
OUTCOME_TRY(decoded, decode(encoded));
```
