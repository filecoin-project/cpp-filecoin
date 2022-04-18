/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

package main

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"reflect"
	"runtime/debug"

	"github.com/filecoin-project/go-address"
	"github.com/filecoin-project/go-state-types/abi"
	"github.com/filecoin-project/go-state-types/cbor"
	"github.com/filecoin-project/go-state-types/crypto"
	"github.com/filecoin-project/go-state-types/exitcode"
	"github.com/filecoin-project/go-state-types/network"
	rtt "github.com/filecoin-project/go-state-types/rt"
	builtin1 "github.com/filecoin-project/specs-actors/actors/builtin"
	account1 "github.com/filecoin-project/specs-actors/actors/builtin/account"
	cron1 "github.com/filecoin-project/specs-actors/actors/builtin/cron"
	init1 "github.com/filecoin-project/specs-actors/actors/builtin/init"
	market1 "github.com/filecoin-project/specs-actors/actors/builtin/market"
	miner1 "github.com/filecoin-project/specs-actors/actors/builtin/miner"
	multisig1 "github.com/filecoin-project/specs-actors/actors/builtin/multisig"
	paych1 "github.com/filecoin-project/specs-actors/actors/builtin/paych"
	power1 "github.com/filecoin-project/specs-actors/actors/builtin/power"
	reward1 "github.com/filecoin-project/specs-actors/actors/builtin/reward"
	system1 "github.com/filecoin-project/specs-actors/actors/builtin/system"
	verifreg1 "github.com/filecoin-project/specs-actors/actors/builtin/verifreg"
	rt1 "github.com/filecoin-project/specs-actors/actors/runtime"
	proof1 "github.com/filecoin-project/specs-actors/actors/runtime/proof"
	builtin2 "github.com/filecoin-project/specs-actors/v2/actors/builtin"
	account2 "github.com/filecoin-project/specs-actors/v2/actors/builtin/account"
	cron2 "github.com/filecoin-project/specs-actors/v2/actors/builtin/cron"
	init2 "github.com/filecoin-project/specs-actors/v2/actors/builtin/init"
	market2 "github.com/filecoin-project/specs-actors/v2/actors/builtin/market"
	miner2 "github.com/filecoin-project/specs-actors/v2/actors/builtin/miner"
	multisig2 "github.com/filecoin-project/specs-actors/v2/actors/builtin/multisig"
	paych2 "github.com/filecoin-project/specs-actors/v2/actors/builtin/paych"
	power2 "github.com/filecoin-project/specs-actors/v2/actors/builtin/power"
	reward2 "github.com/filecoin-project/specs-actors/v2/actors/builtin/reward"
	system2 "github.com/filecoin-project/specs-actors/v2/actors/builtin/system"
	verifreg2 "github.com/filecoin-project/specs-actors/v2/actors/builtin/verifreg"
	rt2 "github.com/filecoin-project/specs-actors/v2/actors/runtime"
	builtin3 "github.com/filecoin-project/specs-actors/v3/actors/builtin"
	account3 "github.com/filecoin-project/specs-actors/v3/actors/builtin/account"
	cron3 "github.com/filecoin-project/specs-actors/v3/actors/builtin/cron"
	init3 "github.com/filecoin-project/specs-actors/v3/actors/builtin/init"
	market3 "github.com/filecoin-project/specs-actors/v3/actors/builtin/market"
	miner3 "github.com/filecoin-project/specs-actors/v3/actors/builtin/miner"
	multisig3 "github.com/filecoin-project/specs-actors/v3/actors/builtin/multisig"
	paych3 "github.com/filecoin-project/specs-actors/v3/actors/builtin/paych"
	power3 "github.com/filecoin-project/specs-actors/v3/actors/builtin/power"
	reward3 "github.com/filecoin-project/specs-actors/v3/actors/builtin/reward"
	system3 "github.com/filecoin-project/specs-actors/v3/actors/builtin/system"
	verifreg3 "github.com/filecoin-project/specs-actors/v3/actors/builtin/verifreg"
	rt3 "github.com/filecoin-project/specs-actors/v3/actors/runtime"
	builtin4 "github.com/filecoin-project/specs-actors/v4/actors/builtin"
	account4 "github.com/filecoin-project/specs-actors/v4/actors/builtin/account"
	cron4 "github.com/filecoin-project/specs-actors/v4/actors/builtin/cron"
	init4 "github.com/filecoin-project/specs-actors/v4/actors/builtin/init"
	market4 "github.com/filecoin-project/specs-actors/v4/actors/builtin/market"
	miner4 "github.com/filecoin-project/specs-actors/v4/actors/builtin/miner"
	multisig4 "github.com/filecoin-project/specs-actors/v4/actors/builtin/multisig"
	paych4 "github.com/filecoin-project/specs-actors/v4/actors/builtin/paych"
	power4 "github.com/filecoin-project/specs-actors/v4/actors/builtin/power"
	reward4 "github.com/filecoin-project/specs-actors/v4/actors/builtin/reward"
	system4 "github.com/filecoin-project/specs-actors/v4/actors/builtin/system"
	verifreg4 "github.com/filecoin-project/specs-actors/v4/actors/builtin/verifreg"
	rt4 "github.com/filecoin-project/specs-actors/v4/actors/runtime"
	builtin5 "github.com/filecoin-project/specs-actors/v5/actors/builtin"
	account5 "github.com/filecoin-project/specs-actors/v5/actors/builtin/account"
	cron5 "github.com/filecoin-project/specs-actors/v5/actors/builtin/cron"
	init5 "github.com/filecoin-project/specs-actors/v5/actors/builtin/init"
	market5 "github.com/filecoin-project/specs-actors/v5/actors/builtin/market"
	miner5 "github.com/filecoin-project/specs-actors/v5/actors/builtin/miner"
	multisig5 "github.com/filecoin-project/specs-actors/v5/actors/builtin/multisig"
	paych5 "github.com/filecoin-project/specs-actors/v5/actors/builtin/paych"
	power5 "github.com/filecoin-project/specs-actors/v5/actors/builtin/power"
	reward5 "github.com/filecoin-project/specs-actors/v5/actors/builtin/reward"
	system5 "github.com/filecoin-project/specs-actors/v5/actors/builtin/system"
	verifreg5 "github.com/filecoin-project/specs-actors/v5/actors/builtin/verifreg"
	rt5 "github.com/filecoin-project/specs-actors/v5/actors/runtime"
	proof5 "github.com/filecoin-project/specs-actors/v5/actors/runtime/proof"
	builtin6 "github.com/filecoin-project/specs-actors/v6/actors/builtin"
	account6 "github.com/filecoin-project/specs-actors/v6/actors/builtin/account"
	cron6 "github.com/filecoin-project/specs-actors/v6/actors/builtin/cron"
	init6 "github.com/filecoin-project/specs-actors/v6/actors/builtin/init"
	market6 "github.com/filecoin-project/specs-actors/v6/actors/builtin/market"
	miner6 "github.com/filecoin-project/specs-actors/v6/actors/builtin/miner"
	multisig6 "github.com/filecoin-project/specs-actors/v6/actors/builtin/multisig"
	paych6 "github.com/filecoin-project/specs-actors/v6/actors/builtin/paych"
	power6 "github.com/filecoin-project/specs-actors/v6/actors/builtin/power"
	reward6 "github.com/filecoin-project/specs-actors/v6/actors/builtin/reward"
	system6 "github.com/filecoin-project/specs-actors/v6/actors/builtin/system"
	verifreg6 "github.com/filecoin-project/specs-actors/v6/actors/builtin/verifreg"
	rt6 "github.com/filecoin-project/specs-actors/v6/actors/runtime"
	builtin7 "github.com/filecoin-project/specs-actors/v7/actors/builtin"
	account7 "github.com/filecoin-project/specs-actors/v7/actors/builtin/account"
	cron7 "github.com/filecoin-project/specs-actors/v7/actors/builtin/cron"
	init7 "github.com/filecoin-project/specs-actors/v7/actors/builtin/init"
	market7 "github.com/filecoin-project/specs-actors/v7/actors/builtin/market"
	miner7 "github.com/filecoin-project/specs-actors/v7/actors/builtin/miner"
	multisig7 "github.com/filecoin-project/specs-actors/v7/actors/builtin/multisig"
	paych7 "github.com/filecoin-project/specs-actors/v7/actors/builtin/paych"
	power7 "github.com/filecoin-project/specs-actors/v7/actors/builtin/power"
	reward7 "github.com/filecoin-project/specs-actors/v7/actors/builtin/reward"
	system7 "github.com/filecoin-project/specs-actors/v7/actors/builtin/system"
	verifreg7 "github.com/filecoin-project/specs-actors/v7/actors/builtin/verifreg"
	rt7 "github.com/filecoin-project/specs-actors/v7/actors/runtime"
	proof7 "github.com/filecoin-project/specs-actors/v7/actors/runtime/proof"
	"github.com/ipfs/go-cid"
	"github.com/whyrusleeping/cbor-gen"
)

//#include "c_actors.h"
import "C"

const ExitFatal = exitcode.ExitCode(-1)

func abort(exit exitcode.ExitCode) {
	panic(exit)
}

type abortf struct {
	exit exitcode.ExitCode
	text string
}

var empty = func() cid.Cid {
	c, e := abi.CidBuilder.Sum([]byte{0x80})
	if e != nil {
		panic(e)
	}
	return c
}()

type rt struct {
	id       uint64
	version  uint64
	base_fee abi.TokenAmount
	from, to address.Address
	now      int64
	value    abi.TokenAmount
	tx       bool
	cv       bool
	ctx      context.Context
}

var _ rt1.Runtime = &rt{}
var _ rt2.Runtime = &rt{}
var _ rt3.Runtime = &rt{}
var _ rt4.Runtime = &rt{}
var _ rt5.Runtime = &rt{}
var _ rt6.Runtime = &rt{}
var _ rt7.Runtime = &rt{}

func (rt *rt) NetworkVersion() network.Version {
	return network.Version(rt.version)
}

func (rt *rt) CurrEpoch() abi.ChainEpoch {
	return abi.ChainEpoch(rt.now)
}

func (rt *rt) ValidateImmediateCallerAcceptAny() {
	rt.validateCallerOnce()
}

func (rt *rt) ValidateImmediateCallerIs(addrs ...address.Address) {
	rt.validateCallerOnce()
	for _, a := range addrs {
		if rt.Caller() == a {
			return
		}
	}
	rt.Abort(exitcode.SysErrForbidden)
}

func (rt *rt) ValidateImmediateCallerType(codes ...cid.Cid) {
	rt.validateCallerOnce()
	code, ok := rt.GetActorCodeCID(rt.Caller())
	if !ok {
		rt.Abort(ExitFatal)
	}
	for _, c := range codes {
		if code == c {
			return
		}
	}
	rt.Abort(exitcode.SysErrForbidden)
}

func (rt *rt) CurrentBalance() abi.TokenAmount {
	ret := rt.gocRet(C.gocRtActorBalance(rt.gocArg().arg()))
	return ret.big()
}

func (rt *rt) ResolveAddress(addr address.Address) (address.Address, bool) {
	ret := rt.gocRet(C.gocRtActorId(rt.gocArg().addr(addr).arg()))
	if ret.bool() {
		return ret.addr(), true
	}
	return address.Undef, false
}

func (rt *rt) GetActorCodeCID(addr address.Address) (cid.Cid, bool) {
	ret := rt.gocRet(C.gocRtActorCode(rt.gocArg().addr(addr).arg()))
	if ret.bool() {
		return ret.cid(), true
	}
	return cid.Undef, false
}

func (rt *rt) GetRandomnessFromBeacon(tag crypto.DomainSeparationTag, round abi.ChainEpoch, seed []byte) abi.Randomness {
	ret := rt.gocRet(C.gocRtRandomnessFromBeacon(rt.gocArg().int(int64(tag)).int(int64(round)).bytes(seed).arg()))
	return ret.bytes()
}

func (rt *rt) GetRandomnessFromTickets(tag crypto.DomainSeparationTag, round abi.ChainEpoch, seed []byte) abi.Randomness {
	ret := rt.gocRet(C.gocRtRandomnessFromTickets(rt.gocArg().int(int64(tag)).int(int64(round)).bytes(seed).arg()))
	return ret.bytes()
}

func (rt *rt) Send(to address.Address, method abi.MethodNum, o cbor.Marshaler, value abi.TokenAmount, out cbor.Er) exitcode.ExitCode {
	if rt.tx {
		rt.Abort(exitcode.SysErrorIllegalActor)
	}
	var params []byte
	if o != nil {
		w := new(bytes.Buffer)
		if e := o.MarshalCBOR(w); e != nil {
			rt.Abort(exitcode.ErrSerialization)
		}
		params = w.Bytes()
	}
	ret := rt.gocRet(C.gocRtSend(rt.gocArg().addr(to).uint(uint64(method)).bytes(params).big(value).arg()))
	exit := exitcode.ExitCode(ret.int())
	if exit == 0 {
		if out.UnmarshalCBOR(bytes.NewReader(ret.bytes())) != nil {
			rt.Abort(exitcode.ErrSerialization)
		}
		return exit
	}
	return exit
}

func (rt *rt) Abortf(exit exitcode.ExitCode, msg string, args ...interface{}) {
	s := fmt.Sprintf("Abort: "+msg, args...)
	if s[len(s)-1] == '\n' {
		s = s[:len(s)-1]
	}
	panic(abortf{exit, s})
}

func (rt *rt) NewActorAddress() address.Address {
	ret := rt.gocRet(C.gocRtNewAddress(rt.gocArg().arg()))
	return ret.addr()
}

func (rt *rt) CreateActor(code cid.Cid, addr address.Address) {
	if !(builtin7.IsBuiltinActor(code) || builtin6.IsBuiltinActor(code) || builtin5.IsBuiltinActor(code) || builtin4.IsBuiltinActor(code) || builtin3.IsBuiltinActor(code) || builtin2.IsBuiltinActor(code) || builtin1.IsBuiltinActor(code)) || IsSingletonActor(code) {
		rt.Abort(exitcode.SysErrorIllegalArgument)
	}
	rt.gocRet(C.gocRtCreateActor(rt.gocArg().cid(code).addr(addr).arg()))
}

func (rt *rt) DeleteActor(to address.Address) {
	rt.gocRet(C.gocRtDeleteActor(rt.gocArg().addr(to).arg()))
}

func (rt *rt) TotalFilCircSupply() abi.TokenAmount {
	return rt.gocRet(C.gocRtCirc(rt.gocArg().arg())).big()
}

func (rt *rt) Context() context.Context {
	return rt.ctx
}

func (rt *rt) StartSpan(string) (EndSpan func()) {
	panic(cgoErrors("NOT IMPLEMENTED StartSpan"))
}

func (rt *rt) ChargeGas(_ string, gas int64, _ int64) {
	rt.gocRet(C.gocRtCharge(rt.gocArg().int(gas).arg()))
}

var log_level rtt.LogLevel = rtt.DEBUG
var log_levels = []string{"DEBUG", "INFO", "WARN", "ERROR"}

func (rt *rt) Log(level rtt.LogLevel, format string, args ...interface{}) {
	if level < rtt.DEBUG {
		return
	}
	if level > rtt.ERROR {
		level = rtt.ERROR
	}
	if level < log_level {
		return
	}
	msg := fmt.Sprintf("%s %s", log_levels[level+1], fmt.Sprintf(format, args...))
	rt.gocRet(C.gocRtLog(rt.gocArg().str(msg).arg()))
}

var _ rt1.StateHandle = &rt{}
var _ rt2.StateHandle = &rt{}
var _ rt3.StateHandle = &rt{}
var _ rt4.StateHandle = &rt{}
var _ rt5.StateHandle = &rt{}
var _ rt6.StateHandle = &rt{}
var _ rt7.StateHandle = &rt{}

func (rt *rt) StateCreate(o cbor.Marshaler) {
	rt.commit(empty, o)
}

func (rt *rt) StateReadonly(o cbor.Unmarshaler) {
	rt.stateGet(o, exitcode.SysErrorIllegalArgument, false)
}

func (rt *rt) StateTransaction(o cbor.Er, f func()) {
	if o == nil {
		rt.Abort(exitcode.SysErrorIllegalActor)
	}
	base := rt.stateGet(o, exitcode.SysErrorIllegalActor, true)
	rt.tx = true
	f()
	rt.tx = false
	rt.commit(base, o)
}

var _ rt1.Store = &rt{}
var _ rt2.Store = &rt{}
var _ rt3.Store = &rt{}
var _ rt4.Store = &rt{}
var _ rt5.Store = &rt{}
var _ rt6.Store = &rt{}
var _ rt7.Store = &rt{}

func (rt *rt) StoreGet(c cid.Cid, o cbor.Unmarshaler) bool {
	ret := rt.gocRet(C.gocRtIpldGet(rt.gocArg().cid(c).arg()))
	if e := o.UnmarshalCBOR(bytes.NewReader(ret.bytes())); e != nil {
		rt.Abort(ExitFatal)
	}
	return true
}

func (rt *rt) StorePut(o cbor.Marshaler) cid.Cid {
	w := new(bytes.Buffer)
	if e := o.MarshalCBOR(w); e != nil {
		rt.Abort(exitcode.ErrSerialization)
	}
	ret := rt.gocRet(C.gocRtIpldPut(rt.gocArg().bytes(w.Bytes()).arg()))
	return ret.cid()
}

var _ rt1.Message = &rt{}
var _ rt2.Message = &rt{}
var _ rt3.Message = &rt{}
var _ rt4.Message = &rt{}
var _ rt5.Message = &rt{}
var _ rt6.Message = &rt{}
var _ rt7.Message = &rt{}

func (rt *rt) Caller() address.Address {
	return rt.from
}

func (rt *rt) Receiver() address.Address {
	return rt.to
}

func (rt *rt) ValueReceived() abi.TokenAmount {
	return rt.value
}

var _ rt1.Syscalls = &rt{}
var _ rt2.Syscalls = &rt{}
var _ rt3.Syscalls = &rt{}
var _ rt4.Syscalls = &rt{}
var _ rt5.Syscalls = &rt{}
var _ rt6.Syscalls = &rt{}
var _ rt7.Syscalls = &rt{}

func (rt *rt) VerifySignature(sig crypto.Signature, addr address.Address, input []byte) error {
	b, e := sig.MarshalBinary()
	if e != nil {
		panic(cgoErrors("VerifySignature MarshalBinary"))
	}
	ret := rt.gocRet(C.gocRtVerifySig(rt.gocArg().bytes(b).addr(addr).bytes(input).arg()))
	if ret.bool() {
		return nil
	}
	return errors.New("VerifySignature")
}

func (rt *rt) HashBlake2b(input []byte) (hash [32]byte) {
	ret := rt.gocRet(C.gocRtBlake(rt.gocArg().bytes(input).arg()))
	copy(hash[:], ret.bytes())
	return
}

func (rt *rt) ComputeUnsealedSectorCID(reg abi.RegisteredSealProof, ps []abi.PieceInfo) (cid.Cid, error) {
	arg := rt.gocArg().int(int64(reg))
	if typegen.CborWriteHeader(arg.w, typegen.MajArray, uint64(len(ps))) != nil {
		panic(cgoErrors("ComputeUnsealedSectorCID CborWriteHeader"))
	}
	for _, p := range ps {
		if p.MarshalCBOR(arg.w) != nil {
			panic(cgoErrors("ComputeUnsealedSectorCID MarshalCBOR"))
		}
	}
	ret := rt.gocRet(C.gocRtCommD(arg.arg()))
	if ret.bool() {
		return ret.cid(), nil
	}
	return cid.Undef, errors.New("ComputeUnsealedSectorCID")
}

func (rt *rt) VerifySeal(proof1.SealVerifyInfo) error {
	// TODO: implement
	panic(cgoErrors("NOT IMPLEMENTED VerifySeal"))
}

func (rt *rt) BatchVerifySeals(batch map[address.Address][]proof1.SealVerifyInfo) (map[address.Address][]bool, error) {
	n := 0
	for _, seals := range batch {
		n += len(seals)
	}
	arg := rt.gocArg().int(int64(n))
	miners := make([]address.Address, 0, len(batch))
	for miner, seals := range batch {
		miners = append(miners, miner)
		for _, seal := range seals {
			if e := seal.MarshalCBOR(arg.w); e != nil {
				panic(cgoErrors("BatchVerifySeals MarshalCBOR"))
			}
		}
	}
	ret := rt.gocRet(C.gocRtVerifySeals(arg.arg()))
	out := make(map[address.Address][]bool)
	for _, miner := range miners {
		seals := batch[miner]
		out[miner] = make([]bool, len(seals))
		for i := range seals {
			out[miner][i] = ret.bool()
		}
	}
	return out, nil
}

var cborArray4 = []byte{132}
var cborArray5 = []byte{133}

func MarshalCBOR_AggregateSealVerifyProofAndInfos(out *cborOut, t *proof5.AggregateSealVerifyProofAndInfos) error {
	w := out.w
	if _, e := w.Write(cborArray5); e != nil {
		return e
	}
	s := make([]byte, 9)
	if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajUnsignedInt, uint64(t.Miner)); e != nil {
		return e
	}
	if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajUnsignedInt, uint64(t.SealProof)); e != nil {
		return e
	}
	if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajUnsignedInt, uint64(t.AggregateProof)); e != nil {
		return e
	}
	if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajByteString, uint64(len(t.Proof))); e != nil {
		return e
	}
	if _, e := w.Write(t.Proof[:]); e != nil {
		return e
	}
	if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajArray, uint64(len(t.Infos))); e != nil {
		return e
	}
	for _, v := range t.Infos {
		if _, e := w.Write(cborArray5); e != nil {
			return e
		}
		if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajUnsignedInt, uint64(v.Number)); e != nil {
			return e
		}
		if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajByteString, uint64(len(v.Randomness))); e != nil {
			return e
		}
		if _, e := w.Write(v.Randomness[:]); e != nil {
			return e
		}
		if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajByteString, uint64(len(v.InteractiveRandomness))); e != nil {
			return e
		}
		if _, e := w.Write(v.InteractiveRandomness[:]); e != nil {
			return e
		}
		if e := typegen.WriteCid(w, v.SealedCID); e != nil {
			return e
		}
		if e := typegen.WriteCid(w, v.UnsealedCID); e != nil {
			return e
		}
	}
	return nil
}

func MarshalCBOR_WindowPoStVerifyInfo(out *cborOut, t *proof1.WindowPoStVerifyInfo) error {
	w := out.w
	if _, e := w.Write(cborArray4); e != nil {
		return e
	}
	s := make([]byte, 9)
	if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajByteString, uint64(len(t.Randomness))); e != nil {
		return e
	}
	if _, e := w.Write(t.Randomness[:]); e != nil {
		return e
	}
	if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajArray, uint64(len(t.Proofs))); e != nil {
		return e
	}
	for _, v := range t.Proofs {
		if e := v.MarshalCBOR(w); e != nil {
			return e
		}
	}
	if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajArray, uint64(len(t.ChallengedSectors))); e != nil {
		return e
	}
	for _, v := range t.ChallengedSectors {
		if e := v.MarshalCBOR(w); e != nil {
			return e
		}
	}
	if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajUnsignedInt, uint64(t.Prover)); e != nil {
		return e
	}
	return nil
}

func MarshalCBOR_ReplicaUpdateInfo(out *cborOut, t *proof7.ReplicaUpdateInfo) error {
	w := out.w
	if _, e := w.Write(cborArray5); e != nil {
		return e
	}
	s := make([]byte, 9)
	if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajUnsignedInt, uint64(t.UpdateProofType)); e != nil {
		return e
	}
	if e := typegen.WriteCid(w, t.OldSealedSectorCID); e != nil {
		return e
	}
	if e := typegen.WriteCid(w, t.NewSealedSectorCID); e != nil {
		return e
	}
	if e := typegen.WriteCid(w, t.NewUnsealedSectorCID); e != nil {
		return e
	}
	if e := typegen.WriteMajorTypeHeaderBuf(s, w, typegen.MajByteString, uint64(len(t.Proof))); e != nil {
		return e
	}
	if _, e := w.Write(t.Proof[:]); e != nil {
		return e
	}
	return nil
}

func (rt *rt) VerifyPoSt(info proof1.WindowPoStVerifyInfo) error {
	arg := rt.gocArg()
	if e := MarshalCBOR_WindowPoStVerifyInfo(arg, &info); e != nil {
		panic(cgoErrors("VerifyPoSt MarshalCBOR"))
	}
	ret := rt.gocRet(C.gocRtVerifyPost(arg.arg()))
	if ret.bool() {
		return nil
	}
	return errors.New("VerifyPost")
}

func (rt *rt) VerifyAggregateSeals(aggregate proof5.AggregateSealVerifyProofAndInfos) error {
	arg := rt.gocArg()
	if e := MarshalCBOR_AggregateSealVerifyProofAndInfos(arg, &aggregate); e != nil {
		panic(cgoErrors("VerifyAggregateSeals MarshalCBOR"))
	}
	ret := rt.gocRet(C.gocRtVerifyAggregateSeals(arg.arg()))
	if ret.bool() {
		return nil
	}
	return exitcode.ErrIllegalArgument
}

func (rt *rt) VerifyReplicaUpdate(info proof7.ReplicaUpdateInfo) error {
	arg := rt.gocArg()
	if e := MarshalCBOR_ReplicaUpdateInfo(arg, &info); e != nil {
		panic(cgoErrors("VerifyReplicaUpdate MarshalCBOR"))
	}
	ret := rt.gocRet(C.gocRtVerifyReplicaUpdate(arg.arg()))
	if ret.bool() {
		return nil
	}
	return exitcode.ErrIllegalArgument
}

func (rt *rt) VerifyConsensusFault(block1, block2, extra []byte) (*rt1.ConsensusFault, error) {
	ret := rt.gocRet(C.gocRtVerifyConsensusFault(rt.gocArg().bytes(block1).bytes(block2).bytes(extra).arg()))
	if ret.bool() {
		target_id, epoch, _type := ret.uint(), abi.ChainEpoch(ret.int()), rt1.ConsensusFaultType(ret.int())
		target, e := address.NewIDAddress(target_id)
		if e != nil {
			panic(cgoErrors("VerifyConsensusFault NewIDAddress"))
		}
		return &rt1.ConsensusFault{target, epoch, _type}, nil
	}
	return nil, errors.New("VerifyConsensusFault")
}

func (rt *rt) Abort(exit exitcode.ExitCode) {
	abort(exit)
}

func (rt *rt) BaseFee() abi.TokenAmount {
	return rt.base_fee
}

func (rt *rt) stateGet(o cbor.Unmarshaler, exit exitcode.ExitCode, cid_ bool) cid.Cid {
	ret := rt.gocRet(C.gocRtStateGet(rt.gocArg().bool(cid_).arg()))
	if ret.bool() {
		if e := o.UnmarshalCBOR(bytes.NewReader(ret.bytes())); e != nil {
			rt.Abort(ExitFatal)
		}
		if cid_ {
			return ret.cid()
		}
	} else {
		rt.Abort(exit)
	}
	return cid.Undef
}

func (rt *rt) commit(base cid.Cid, o cbor.Marshaler) {
	w := new(bytes.Buffer)
	if e := o.MarshalCBOR(w); e != nil {
		rt.Abort(exitcode.ErrSerialization)
	}
	rt.gocRet(C.gocRtStateCommit(rt.gocArg().bytes(w.Bytes()).cid(base).arg()))
}

func (rt *rt) validateCallerOnce() {
	if rt.cv {
		rt.Abort(exitcode.SysErrorIllegalActor)
	}
	rt.cv = true
}

func (rt *rt) gocArg() *cborOut {
	return CborOut().uint(rt.id)
}

func (rt *rt) gocRet(raw C.Raw) *cborIn {
	ret := CborIn(gocRet(raw))
	exit := exitcode.ExitCode(ret.int())
	if exit != exitcode.Ok {
		rt.Abort(exit)
	}
	return ret
}

func IsSingletonActor(code cid.Cid) bool {
	actor, ok := _actors[code]
	return ok && rtt.IsSingletonActor(actor)
}

type exporter interface{ Exports() []interface{} }
type method = func(rt *rt, params []byte) []byte
type methods = map[uint64]method

var _actors = map[cid.Cid]rtt.VMActor{
	builtin1.SystemActorCodeID:           system1.Actor{},
	builtin1.InitActorCodeID:             init1.Actor{},
	builtin1.RewardActorCodeID:           reward1.Actor{},
	builtin1.CronActorCodeID:             cron1.Actor{},
	builtin1.StoragePowerActorCodeID:     power1.Actor{},
	builtin1.StorageMarketActorCodeID:    market1.Actor{},
	builtin1.StorageMinerActorCodeID:     miner1.Actor{},
	builtin1.MultisigActorCodeID:         multisig1.Actor{},
	builtin1.PaymentChannelActorCodeID:   paych1.Actor{},
	builtin1.VerifiedRegistryActorCodeID: verifreg1.Actor{},
	builtin1.AccountActorCodeID:          account1.Actor{},

	builtin2.SystemActorCodeID:           system2.Actor{},
	builtin2.InitActorCodeID:             init2.Actor{},
	builtin2.RewardActorCodeID:           reward2.Actor{},
	builtin2.CronActorCodeID:             cron2.Actor{},
	builtin2.StoragePowerActorCodeID:     power2.Actor{},
	builtin2.StorageMarketActorCodeID:    market2.Actor{},
	builtin2.StorageMinerActorCodeID:     miner2.Actor{},
	builtin2.MultisigActorCodeID:         multisig2.Actor{},
	builtin2.PaymentChannelActorCodeID:   paych2.Actor{},
	builtin2.VerifiedRegistryActorCodeID: verifreg2.Actor{},
	builtin2.AccountActorCodeID:          account2.Actor{},

	builtin3.SystemActorCodeID:           system3.Actor{},
	builtin3.InitActorCodeID:             init3.Actor{},
	builtin3.RewardActorCodeID:           reward3.Actor{},
	builtin3.CronActorCodeID:             cron3.Actor{},
	builtin3.StoragePowerActorCodeID:     power3.Actor{},
	builtin3.StorageMarketActorCodeID:    market3.Actor{},
	builtin3.StorageMinerActorCodeID:     miner3.Actor{},
	builtin3.MultisigActorCodeID:         multisig3.Actor{},
	builtin3.PaymentChannelActorCodeID:   paych3.Actor{},
	builtin3.VerifiedRegistryActorCodeID: verifreg3.Actor{},
	builtin3.AccountActorCodeID:          account3.Actor{},

	builtin4.SystemActorCodeID:           system4.Actor{},
	builtin4.InitActorCodeID:             init4.Actor{},
	builtin4.RewardActorCodeID:           reward4.Actor{},
	builtin4.CronActorCodeID:             cron4.Actor{},
	builtin4.StoragePowerActorCodeID:     power4.Actor{},
	builtin4.StorageMarketActorCodeID:    market4.Actor{},
	builtin4.StorageMinerActorCodeID:     miner4.Actor{},
	builtin4.MultisigActorCodeID:         multisig4.Actor{},
	builtin4.PaymentChannelActorCodeID:   paych4.Actor{},
	builtin4.VerifiedRegistryActorCodeID: verifreg4.Actor{},
	builtin4.AccountActorCodeID:          account4.Actor{},

	builtin5.SystemActorCodeID:           system5.Actor{},
	builtin5.InitActorCodeID:             init5.Actor{},
	builtin5.RewardActorCodeID:           reward5.Actor{},
	builtin5.CronActorCodeID:             cron5.Actor{},
	builtin5.StoragePowerActorCodeID:     power5.Actor{},
	builtin5.StorageMarketActorCodeID:    market5.Actor{},
	builtin5.StorageMinerActorCodeID:     miner5.Actor{},
	builtin5.MultisigActorCodeID:         multisig5.Actor{},
	builtin5.PaymentChannelActorCodeID:   paych5.Actor{},
	builtin5.VerifiedRegistryActorCodeID: verifreg5.Actor{},
	builtin5.AccountActorCodeID:          account5.Actor{},

	builtin6.SystemActorCodeID:           system6.Actor{},
	builtin6.InitActorCodeID:             init6.Actor{},
	builtin6.RewardActorCodeID:           reward6.Actor{},
	builtin6.CronActorCodeID:             cron6.Actor{},
	builtin6.StoragePowerActorCodeID:     power6.Actor{},
	builtin6.StorageMarketActorCodeID:    market6.Actor{},
	builtin6.StorageMinerActorCodeID:     miner6.Actor{},
	builtin6.MultisigActorCodeID:         multisig6.Actor{},
	builtin6.PaymentChannelActorCodeID:   paych6.Actor{},
	builtin6.VerifiedRegistryActorCodeID: verifreg6.Actor{},
	builtin6.AccountActorCodeID:          account6.Actor{},

	builtin7.SystemActorCodeID:           system7.Actor{},
	builtin7.InitActorCodeID:             init7.Actor{},
	builtin7.RewardActorCodeID:           reward7.Actor{},
	builtin7.CronActorCodeID:             cron7.Actor{},
	builtin7.StoragePowerActorCodeID:     power7.Actor{},
	builtin7.StorageMarketActorCodeID:    market7.Actor{},
	builtin7.StorageMinerActorCodeID:     miner7.Actor{},
	builtin7.MultisigActorCodeID:         multisig7.Actor{},
	builtin7.PaymentChannelActorCodeID:   paych7.Actor{},
	builtin7.VerifiedRegistryActorCodeID: verifreg7.Actor{},
	builtin7.AccountActorCodeID:          account7.Actor{},
}
var actors = map[cid.Cid]methods{}

func export(exporter exporter) methods {
	methods := make(methods)
	for i, method := range exporter.Exports() {
		if method != nil {
			rMethod := reflect.ValueOf(method)
			methods[uint64(i)] = func(rt *rt, params []byte) []byte {
				rParams := reflect.New(rMethod.Type().In(1).Elem())
				e := rParams.Interface().(typegen.CBORUnmarshaler).UnmarshalCBOR(bytes.NewReader(params))
				if e != nil {
					if rt.NetworkVersion() >= network.Version7 {
						abort(exitcode.ErrSerialization)
					}
					abort(exitcode.ExitCode(1))
				}
				w := new(bytes.Buffer)
				rResult := rMethod.Call([]reflect.Value{reflect.ValueOf(rt), rParams})[0]
				if !rt.cv {
					abort(exitcode.SysErrorIllegalActor)
				}
				e = rResult.Interface().(typegen.CBORMarshaler).MarshalCBOR(w)
				if e != nil {
					abort(exitcode.ExitCode(2))
				}
				return w.Bytes()
			}
		}
	}
	return methods
}

func invoke(rt *rt, code cid.Cid, method uint64, params []byte) (exit exitcode.ExitCode, ret []byte) {
	methods, ok := actors[code]
	if !ok {
		return exitcode.SysErrorIllegalActor, nil
	}
	f, ok := methods[method]
	if !ok {
		return exitcode.SysErrInvalidMethod, nil
	}
	defer func() {
		if e := recover(); e != nil {
			if c, ok := e.(cgoError); ok {
				fmt.Println("[go_actors] invoke cgoError", c.e)
				e = c.e
				exit = ExitFatal
			} else if abortf, ok := e.(abortf); ok {
				exit = abortf.exit
				ret = []byte(abortf.text)
			} else if exit, ok = e.(exitcode.ExitCode); !ok {
				exit = exitcode.ExitCode(1)
			}
			if exit == ExitFatal {
				fmt.Println("[go_actors] invoke fatal", e)
				debug.PrintStack()
			}
		}
	}()
	return exitcode.Ok, f(rt, params)
}

//export cgoActorsInvoke
func cgoActorsInvoke(raw C.Raw) C.Raw {
	arg := cgoArgCbor(raw)
	id, version, base_fee, from, to, now, value, code, method, params := arg.uint(), arg.uint(), arg.big(), arg.addr(), arg.addr(), arg.int(), arg.big(), arg.cid(), arg.uint(), arg.bytes()
	exit, ret := invoke(&rt{id, version, base_fee, from, to, now, value, false, false, context.Background()}, code, method, params)
	return CborOut().int(int64(exit)).bytes(ret).ret()
}

func ClearSupportedProofTypes(n int) {
	miner1.SupportedProofTypes = make(map[abi.RegisteredSealProof]struct{}, n)

	miner2.PreCommitSealProofTypesV0 = make(map[abi.RegisteredSealProof]struct{}, n)
	miner2.PreCommitSealProofTypesV7 = make(map[abi.RegisteredSealProof]struct{}, n*2)
	miner2.PreCommitSealProofTypesV8 = make(map[abi.RegisteredSealProof]struct{}, n)

	miner3.PreCommitSealProofTypesV0 = make(map[abi.RegisteredSealProof]struct{}, n)
	miner3.PreCommitSealProofTypesV7 = make(map[abi.RegisteredSealProof]struct{}, n*2)
	miner3.PreCommitSealProofTypesV8 = make(map[abi.RegisteredSealProof]struct{}, n)

	miner4.PreCommitSealProofTypesV0 = make(map[abi.RegisteredSealProof]struct{}, n)
	miner4.PreCommitSealProofTypesV7 = make(map[abi.RegisteredSealProof]struct{}, n*2)
	miner4.PreCommitSealProofTypesV8 = make(map[abi.RegisteredSealProof]struct{}, n)

	miner5.PreCommitSealProofTypesV8 = make(map[abi.RegisteredSealProof]struct{}, n)

	miner6.PreCommitSealProofTypesV8 = make(map[abi.RegisteredSealProof]struct{}, n)

	miner7.PreCommitSealProofTypesV8 = make(map[abi.RegisteredSealProof]struct{}, n)
}

func AddSupportedProofTypes(t abi.RegisteredSealProof) {
	if t >= abi.RegisteredSealProof_StackedDrg2KiBV1_1 {
		panic(cgoErrors("AddSupportedProofTypes: must specify v1 proof types only"))
	}

	miner1.SupportedProofTypes[t] = struct{}{}

	miner2.PreCommitSealProofTypesV0[t] = struct{}{}
	miner2.PreCommitSealProofTypesV7[t] = struct{}{}
	miner2.PreCommitSealProofTypesV7[t+abi.RegisteredSealProof_StackedDrg2KiBV1_1] = struct{}{}
	miner2.PreCommitSealProofTypesV8[t+abi.RegisteredSealProof_StackedDrg2KiBV1_1] = struct{}{}

	miner3.PreCommitSealProofTypesV0[t] = struct{}{}
	miner3.PreCommitSealProofTypesV7[t] = struct{}{}
	miner3.PreCommitSealProofTypesV7[t+abi.RegisteredSealProof_StackedDrg2KiBV1_1] = struct{}{}
	miner3.PreCommitSealProofTypesV8[t+abi.RegisteredSealProof_StackedDrg2KiBV1_1] = struct{}{}

	miner4.PreCommitSealProofTypesV0[t] = struct{}{}
	miner4.PreCommitSealProofTypesV7[t] = struct{}{}
	miner4.PreCommitSealProofTypesV7[t+abi.RegisteredSealProof_StackedDrg2KiBV1_1] = struct{}{}
	miner4.PreCommitSealProofTypesV8[t+abi.RegisteredSealProof_StackedDrg2KiBV1_1] = struct{}{}

	miner5.PreCommitSealProofTypesV8[t+abi.RegisteredSealProof_StackedDrg2KiBV1_1] = struct{}{}
	miner6.PreCommitSealProofTypesV8[t+abi.RegisteredSealProof_StackedDrg2KiBV1_1] = struct{}{}
	miner7.PreCommitSealProofTypesV8[t+abi.RegisteredSealProof_StackedDrg2KiBV1_1] = struct{}{}

	wpp, e := t.RegisteredWindowPoStProof()
	if e != nil {
		panic(cgoErrors("AddSupportedProofTypes: RegisteredWindowPoStProof"))
	}
	miner5.WindowPoStProofTypes[wpp] = struct{}{}
	miner6.WindowPoStProofTypes[wpp] = struct{}{}
	miner7.WindowPoStProofTypes[wpp] = struct{}{}
}

//export cgoActorsConfigParams
func cgoActorsConfigParams(raw C.Raw) C.Raw {
	arg := cgoArgCbor(raw)

	MinVerifiedDealSize := arg.big()
	verifreg1.MinVerifiedDealSize = MinVerifiedDealSize
	verifreg2.MinVerifiedDealSize = MinVerifiedDealSize
	verifreg3.MinVerifiedDealSize = MinVerifiedDealSize
	verifreg4.MinVerifiedDealSize = MinVerifiedDealSize
	verifreg5.MinVerifiedDealSize = MinVerifiedDealSize
	verifreg6.MinVerifiedDealSize = MinVerifiedDealSize
	verifreg7.MinVerifiedDealSize = MinVerifiedDealSize

	PreCommitChallengeDelay := abi.ChainEpoch(arg.int())
	miner1.PreCommitChallengeDelay = PreCommitChallengeDelay
	miner2.PreCommitChallengeDelay = PreCommitChallengeDelay
	miner3.PreCommitChallengeDelay = PreCommitChallengeDelay
	miner4.PreCommitChallengeDelay = PreCommitChallengeDelay
	miner5.PreCommitChallengeDelay = PreCommitChallengeDelay
	miner6.PreCommitChallengeDelay = PreCommitChallengeDelay
	miner7.PreCommitChallengeDelay = PreCommitChallengeDelay

	power1.ConsensusMinerMinPower = arg.big()
	for _, x := range builtin2.SealProofPolicies {
		x.ConsensusMinerMinPower = power1.ConsensusMinerMinPower
	}
	for _, x := range builtin3.PoStProofPolicies {
		x.ConsensusMinerMinPower = power1.ConsensusMinerMinPower
	}
	for _, x := range builtin4.PoStProofPolicies {
		x.ConsensusMinerMinPower = power1.ConsensusMinerMinPower
	}
	for _, x := range builtin5.PoStProofPolicies {
		x.ConsensusMinerMinPower = power1.ConsensusMinerMinPower
	}
	for _, x := range builtin6.PoStProofPolicies {
		x.ConsensusMinerMinPower = power1.ConsensusMinerMinPower
	}
	for _, x := range builtin7.PoStProofPolicies {
		x.ConsensusMinerMinPower = power1.ConsensusMinerMinPower
	}
	nProofs := int(arg.uint())
	ClearSupportedProofTypes(nProofs)
	for i := 0; i < nProofs; i++ {
		AddSupportedProofTypes(abi.RegisteredSealProof(arg.int()))
	}
	return cgoRet(nil)
}

//export cgoActorsSetLogLevel
func cgoActorsSetLogLevel(raw C.Raw) C.Raw {
	arg := cgoArgCbor(raw)
	log_level = rtt.LogLevel(arg.int())
	return cgoRet(nil)
}

func init() {
	for k, v := range _actors {
		actors[k] = export(v)
	}
}

func main() {}
