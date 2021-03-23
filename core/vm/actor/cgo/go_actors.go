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
	"github.com/ipfs/go-cid"
	"github.com/whyrusleeping/cbor-gen"
)

//#include "c_actors.h"
import "C"

const ExitFatal = exitcode.ExitCode(-1)

func abort(exit exitcode.ExitCode) {
	panic(exit)
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
	if s[len(s)-1] != '\n' {
		s += "\n"
	}
	fmt.Print(s)
	rt.Abort(exit)
}

func (rt *rt) NewActorAddress() address.Address {
	ret := rt.gocRet(C.gocRtNewAddress(rt.gocArg().arg()))
	return ret.addr()
}

func (rt *rt) CreateActor(code cid.Cid, addr address.Address) {
	if !(builtin3.IsBuiltinActor(code) || builtin2.IsBuiltinActor(code) || builtin1.IsBuiltinActor(code)) || IsSingletonActor(code) {
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

func (rt *rt) Log(rtt.LogLevel, string, ...interface{}) {
}

var _ rt1.StateHandle = &rt{}
var _ rt2.StateHandle = &rt{}
var _ rt3.StateHandle = &rt{}

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

var lengthBufWindowPoStVerifyInfo = []byte{132}

func MarshalCBOR(out *cborOut, t *proof1.WindowPoStVerifyInfo) error {
	w := out.w
	if _, e := w.Write(lengthBufWindowPoStVerifyInfo); e != nil {
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

func (rt *rt) VerifyPoSt(info proof1.WindowPoStVerifyInfo) error {
	arg := rt.gocArg()
	if e := MarshalCBOR(arg, &info); e != nil {
		panic(cgoErrors("VerifyPoSt MarshalCBOR"))
	}
	ret := rt.gocRet(C.gocRtVerifyPost(arg.arg()))
	if ret.bool() {
		return nil
	}
	return errors.New("VerifyPost")
}

func (rt *rt) VerifyConsensusFault(block1, block2, extra []byte) (*rt1.ConsensusFault, error) {
	ret := rt.gocRet(C.gocRtVerifyConsensusFault(rt.gocArg().bytes(block1).bytes(block2).bytes(extra).arg()))
	if ret.bool() {
		target, epoch, _type := ret.addr(), abi.ChainEpoch(ret.int()), rt1.ConsensusFaultType(ret.int())
		return &rt1.ConsensusFault{target, epoch, _type}, nil
	}
	return nil, errors.New("VerifyConsensusFault")
}

func (rt *rt) Abort(exit exitcode.ExitCode) {
	abort(exit)
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
	id, version, from, to, now, value, code, method, params := arg.uint(), arg.uint(), arg.addr(), arg.addr(), arg.int(), arg.big(), arg.cid(), arg.uint(), arg.bytes()
	exit, ret := invoke(&rt{id, version, from, to, now, value, false, false, context.Background()}, code, method, params)
	return CborOut().int(int64(exit)).bytes(ret).ret()
}

//export cgoActorsConfigMainnet
func cgoActorsConfigMainnet(C.Raw) C.Raw {
	power1.ConsensusMinerMinPower = abi.NewStoragePower(10 << 40)
	for _, x := range builtin2.SealProofPolicies {
		x.ConsensusMinerMinPower = power1.ConsensusMinerMinPower
	}
	for _, x := range builtin3.PoStProofPolicies {
		x.ConsensusMinerMinPower = power1.ConsensusMinerMinPower
	}
	return cgoRet(nil)
}

func init() {
	for k, v := range _actors {
		actors[k] = export(v)
	}
}

func main() {}
