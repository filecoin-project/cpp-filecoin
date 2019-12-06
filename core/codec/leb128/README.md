# LEB128 codec
> Primitive types encode/decode user guide

### Encoding stream

```c
#include "codec/leb128/leb128_encode_stream.hpp"
using fc::codec::leb128::LEB128EncodeStream;

LEB128EncodeStream encoder;
T data;                      // Some numeric type to encode
encoder << data;
std::vector<uint8_t> output; // Byte-vector for encoded data
encoder >> output;
```

### Decoding stream

```c
#include "codec/leb128/leb128_decode_stream.hpp"
using fc::codec::leb128::LEB128DecodeStream;

std::vector<uint8_t> encoded; // Byte-vector with encoded data
LEB128DecodeStream decoder{ encoded };
T data;                       // Some numeric type for decoded data
decoder >> data;              // Decode can throw exception, use here try/catch
```

### Free functions

```c
#include "codec/leb128/leb128.hpp"
using fc::codec::leb128::encode;
using fc::codec::leb128::decode;

T data;
std::vector<uint8_t> encoded = encode(data);
OUTCOME_TRY(decoded, decode(encoded));
```
