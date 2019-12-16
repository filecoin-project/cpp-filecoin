
CBOR allows encoding and decoding of scalar types, as well as list and maps (with string keys).

```c++
CborEncodeStream e;
CborDecodeStream d;
int int1, int2, int3;

// int, 1, 2
e << 1 << 2;
// decode
d >> int1 >> int2;

// list, [1, 2, 3]
// encode
e << (e.list() << 1 << 2 << 3);
// decode
d.list() >> int1 >> int2 >> int3;

// map, {"a": 1, "b": [2]}
// encode
auto em = e.map();
em["a"] << 1;
em["b"] << (em["b"].list << 2);
// decode
auto dm = d.map();
dm.at("a") >> int1;
dm.at("b").list() >> int2;
```
