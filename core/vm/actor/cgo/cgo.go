/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package main

import (
	"bytes"
	"errors"
	"github.com/filecoin-project/go-address"
	"github.com/filecoin-project/go-state-types/big"
	"github.com/ipfs/go-cid"
	"github.com/whyrusleeping/cbor-gen"
	"math"
	"reflect"
	"unsafe"
)

//#cgo linux LDFLAGS: -Wl,-unresolved-symbols=ignore-all
//#cgo darwin LDFLAGS: -Wl,-undefined,dynamic_lookup
//#include "cgo.hpp"
import "C"

func gocArg(bytes []byte) C.Raw {
	return C.Raw{
		(*C.uchar)(&bytes[0]),
		C.size_t(len(bytes)),
	}
}

func gocRet(raw C.Raw) []byte {
	ptr := unsafe.Pointer(raw.data)
	bytes := C.GoBytes(ptr, C.int(raw.size))
	C.free(ptr)
	return bytes
}

func cgoArg(raw C.Raw) []byte {
	return *(*[]byte)(unsafe.Pointer(&reflect.SliceHeader{
		uintptr(unsafe.Pointer(raw.data)),
		int(raw.size),
		int(raw.size),
	}))
}

func cgoRet(bytes []byte) C.Raw {
	return C.Raw{
		(*C.uchar)(C.CBytes(bytes)),
		C.size_t(len(bytes)),
	}
}

func cgoArgCbor(raw C.Raw) *cborIn {
	return CborIn(cgoArg(raw))
}

type cgoError struct {
	e error
}

func cgoErrors(s string) cgoError {
	return cgoError{errors.New(s)}
}

func cgoAsserte(e error) {
	if e != nil {
		panic(cgoError{e})
	}
}

func cgoAssert(c bool) {
	if !c {
		panic(cgoErrors("assert"))
	}
}

type cborIn struct {
	r typegen.BytePeeker
}

func CborIn(b []byte) *cborIn {
	return &cborIn{typegen.GetPeeker(bytes.NewReader(b))}
}

func (in *cborIn) cid() cid.Cid {
	c, e := typegen.ReadCid(in.r)
	cgoAsserte(e)
	return c
}

func (in *cborIn) uint() uint64 {
	t, v, e := typegen.CborReadHeader(in.r)
	cgoAssert(t == typegen.MajUnsignedInt)
	cgoAsserte(e)
	return v
}

func (in *cborIn) int() int64 {
	t, v, e := typegen.CborReadHeader(in.r)
	cgoAsserte(e)
	if t == typegen.MajNegativeInt {
		return int64(-v - 1)
	} else {
		cgoAssert(t == typegen.MajUnsignedInt)
	}
	return int64(v)
}

func (in *cborIn) bytes() []byte {
	b, e := typegen.ReadByteArray(in.r, math.MaxUint64)
	cgoAsserte(e)
	return b
}

func (in *cborIn) bool() bool {
	b, e := in.r.ReadByte()
	cgoAsserte(e)
	if b == typegen.CborBoolTrue[0] {
		return true
	}
	cgoAssert(b == typegen.CborBoolFalse[0])
	return false
}

func (in *cborIn) big() big.Int {
	b, e := big.FromBytes(in.bytes())
	cgoAsserte(e)
	return b
}

func (in *cborIn) addr() address.Address {
	a, e := address.NewFromBytes(in.bytes())
	cgoAsserte(e)
	return a
}

type cborOut struct {
	w *bytes.Buffer
}

func CborOut() *cborOut {
	return &cborOut{new(bytes.Buffer)}
}

func (out *cborOut) cid(c cid.Cid) *cborOut {
	e := typegen.WriteCid(out.w, c)
	cgoAsserte(e)
	return out
}

func (out *cborOut) bytes(b []byte) *cborOut {
	e := typegen.CborWriteHeader(out.w, typegen.MajByteString, uint64(len(b)))
	cgoAsserte(e)
	_, e = out.w.Write(b)
	cgoAsserte(e)
	return out
}

func (out *cborOut) str(s string) *cborOut {
	e := typegen.CborWriteHeader(out.w, typegen.MajTextString, uint64(len(s)))
	cgoAsserte(e)
	_, e = out.w.Write([]byte(s))
	cgoAsserte(e)
	return out
}

func (out *cborOut) int(i int64) *cborOut {
	t := typegen.MajUnsignedInt
	if i < 0 {
		t = typegen.MajNegativeInt
		i = -i - 1
	}
	e := typegen.CborWriteHeader(out.w, byte(t), uint64(i))
	cgoAsserte(e)
	return out
}

func (out *cborOut) uint(i uint64) *cborOut {
	e := typegen.CborWriteHeader(out.w, typegen.MajUnsignedInt, i)
	cgoAsserte(e)
	return out
}

func (out *cborOut) addr(a address.Address) *cborOut {
	return out.bytes(a.Bytes())
}

func (out *cborOut) big(b big.Int) *cborOut {
	c, e := b.Bytes()
	cgoAsserte(e)
	return out.bytes(c)
}

func (out *cborOut) bool(b bool) *cborOut {
	if b {
		out.w.Write(typegen.CborBoolTrue)
	} else {
		out.w.Write(typegen.CborBoolFalse)
	}
	return out
}

func (out *cborOut) data() []byte {
	return out.w.Bytes()
}

func (out *cborOut) arg() C.Raw {
	return gocArg(out.data())
}

func (out *cborOut) ret() C.Raw {
	return cgoRet(out.data())
}
