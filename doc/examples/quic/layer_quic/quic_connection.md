
---
layout: page
title: QUIC connection protocol
---

This document describes the wire specification of QUIC.  It is based
on the following IETF drafts:

1.[QUIC: A UDP-Based Multiplexed and Secure Transport
draft-ietf-quic-transport-14](https://tools.ietf.org/html/draft-ietf-quic-transport-14).

2. [Using Transport Layer Security (TLS) to Secure QUIC
draft-ietf-quic-tls-14](https://tools.ietf.org/html/draft-ietf-quic-tls-14).

The protocol is modeled as a stack of layers. Each layer
is associated with one or more event types. The highest-level
events in this stack correspond to the sending and receiving of
application data, while the lowest-level events correspond
to UDP datagrams. The layers in the stack are, from top to bottom:

- The application
- The TLS handshake protocol
- The QUIC frame protocol
- The QUIC packet protocol
- QUIC packet protection
- UDP

References
==========

```
include quic_types
include quic_frame
include quic_packet
include tls_protocol
include quic_transport_parameters

```
Protocol state
==============

This section desceibes the history variables that track the state of
the various protocol layers. Some of these variables are sharded between
protocol layers, so that the allowed interleavings of events at different
layers can be specified.


Packet protocol state
---------------------

- For each cid C, `conn_seen(C)` is true if a packet with
  source cid C has been transmitted.

- For each aid C, `conn_closed(C)` is true if C is in the closed state.

- For each endpoint aid C, last_pkt_num(C,L) represents the
  number of the latest packet sent by C in encryption level L.

- For each aid C, sent_pkt(C,L,N) is true if
  a packet numbered N has been sent by C in encryption level L.

- For each aid C, acked_pkt(C,L,N) is true if
  a packet numbered N sent by C in encryption level L has been
  acknowledged. 

- For each aid C, max_acked(C,L) is the greatest
  packet number N such that acked_pkt(C,L,N), or zero if
  forall N. ~acked(C,L,N).

- For each aid C, ack_credit(E,C) is the number
  of non-ack-only packets sent to C, less the number of
  ack-only packets sent from C.

- For each aid C, `trans_params_set(C)`
  is true if C has declared transport parameters.

- For each aid C, function `trans_params(C)`
  gives the transport parameters declared by C.

- The predicate `cid_dst_to_src_set(C)` indicates that the peer of
  aid `C` has been determined.  This occurs conceptually when a
  server produces a server hello frame.

- The function `cid_dst_to_src(C)` maps the aid `C` to its peer, if
  the peer has been determined.

- The predicate `is_client(C)` indicates that aid 'C' is taking
  the client role.

- The predicate `conn_requested(S,D,C)` indicates that a client
  and endpoint S has requested to open a connection with a server
  at endpoint D, using cid C. 

- The function `hi_non_probing(C)` indicates the highest packet number
  of a non-probing packet sent by aid `C`. 

- The relation `hi_non_probing_endpoint(C,E)` that the highest-numbered
  non-probing packet of aid `C` was at some time sent from endpopint `E`.

```
relation conn_seen(C:cid)
relation conn_closed(C:cid)
function last_pkt_num(C:cid,L:encryption_level) : pkt_num
relation sent_pkt(C:cid,L:encryption_level,N:pkt_num)
relation acked_pkt(C:cid,L:encryption_level,N:pkt_num)
function max_acked(C:cid,L:encryption_level) : pkt_num
function ack_credit(C:cid) : pkt_num
relation trans_params_set(C:cid)
function trans_params(C:cid) : trans_params_struct
relation cid_dst_to_src_set(C:cid)
function cid_dst_to_src(C:cid) : cid
relation is_client(C:cid)
relation conn_requested(S:ip.endpoint,D:ip.endpoint,C:cid)
function hi_non_probing(C:cid) : pkt_num
relation hi_non_probing_endpoint(C:cid,E:ip.endpoint)

```
TLS handshake protocol state
----------------------------

- For aid C, `crypto_data(C,L)` represents the crypto handshake data
  transmitted by TLS from C in encyption level L. This data may or
  may not yet have been transmitted in any QUIC packet.

- For aid C, the relation `crypto_data_presents(C,L)` represents the
  set of byte positions that are present in the crypto handshake
  data transmitted by TLS from C in encyption level L. A handshake
  data byte may not be present, even if bytes in higher positions
  are present (in other words, the handshake data may contain
  "holes"). This allows the handshake data bytes to be generated out
  of order, but processed in order.

- For aid C, `crypto_data_end(C,L)` represents the length in bytes
  of the crypto handshake data transmitted by TLS from C in
  encyption level L. That is, it is the position of the highest
  present byte plus one, or zero if there are no present bytes.

- For aid C, `crypto_length(C,L)` indicates the length of the crypto
  handshake data transmitted in QUIC frames from C at encyption
  level L. The length is the least stream position greater than the
  position of all bytes transmitted. Note this may be less than the
  length of the crypto data, but may not be greater.

- For aid C, `crypto_pos(C,L)` represents the read position of the
  crypto data transmitted by C in encyption level L. This is the
  number of bytes in the stream that have been read by TLS at the
  peer.

- For aid C, `crypto_handler_pos(C,L)` represents the position in the
  crypto handshake data sent by C of any partial message at end, or
  if there is no partial message, the end position.

- For each aid C, `conn_enc_level` represents the current encryption
  level of TLS at C.

- The predicate `established_1rtt_keys(C)` holds if TLS has established
  its 1rrt keys at aid C.

```
function crypto_data(C:cid,L:encryption_level) : stream_data
relation crypto_data_present(C:cid,L:encryption_level,P:stream_pos)
function crypto_data_end(C:cid,L:encryption_level) : stream_pos
function crypto_length(C:cid,L:encryption_level) : stream_pos
function crypto_pos(C:cid,L:encryption_level) : stream_pos
function crypto_handler_pos(C:cid,L:encryption_level) : stream_pos
function conn_enc_level(E:ip.endpoint,C:cid) : encryption_level
relation established_1rtt_keys(C:cid)

```
The frame protocol
------------------

- For each aid C,and stream id S, `stream_seen(C,S)`
  indicates that a stream frame for stream id S
  has been sent to C. TODO: change this so it uses the source aid,
  not the destination cid.

- For each aid C,and stream id S, `max_stream_data_val(C,S)`
  indicates the maximum number of bytes that may be sent on stream
  id S to C.

- For each aid C,and stream id S, `max_stream_data_set(C,S)`
  indicates the maximum number of bytes that may be sent on stream
  id S to C has been set.

- For each aid C,and stream id S, `stream_length(C,S)`
  indicates the length of the stream data transmitted in QUIC
  packets on stream id S to cid C. The length is the
  least stream position greater than the position of all bytes
  transmitted. Note this may be less than the length of the application
  data, but may not be greater.

- For aid C,and stream id S, `stream_finished(C,S)` indicates that
  the stream transmitted to C on stream id S is finished (that is, a
  FIN frame has been sent).

- For each aid C,and stream id S, `stream_reset(C,S)` indicates that
  the stream transmitted to C on stream id S is reset (that is, a
  RESET_STREAM frame has been sent).

- For each aid C,and stream kind K, `max_stream_set(C,K)`
  indicates that the maximum stream id has been declared.

- For each aid C,and stream kind K, `max_stream(E,C,K)`
  indicates the declared maximum stream id.

- The queued frames at aid `C` are
  represented by `queued_frames(C)` and are initially empty.

- The relation `queued_hello(C)` indicates that one of the queued
  frames at aid `C` contains a TLS hello record.

- The relation `queued_non_probing(C)` indicates that one of the queued
  frames at aid `C` contains a non-probing frame. This is a frame
  other than path challenge, new connection id and padding.

- The function num_queued_frames(C:cid) gives the number of frames
  queue at aid `C`.

- The predicate `path_challenge_pending(C,D)` that a path challenge
  has been sent to aid C with data D, and has not yet been responded
  to. QUESTION: should path responses be resent, or should the client
  wait for a resent path challenge?

```
relation stream_seen(C:cid,S:stream_id)
function max_stream_data_val(C:cid,S:stream_id) : stream_pos
relation max_stream_data_set(C:cid,S:stream_id)
function stream_length(C:cid,S:stream_id) : stream_pos
relation stream_finished(C:cid,S:stream_id)
relation stream_reset(C:cid,S:stream_id)
relation max_stream_set(C:cid,K:stream_kind)
function max_stream(C:cid,K:stream_kind) : stream_id
function queued_frames(C:cid) : frame.arr
function queued_level(C:cid) : encryption_level
relation queued_hello(C:cid)
relation queued_non_probing(C:cid)
function num_queued_frames(C:cid) : frame.idx
relation path_challenge_pending(C:cid,D:stream_data)

```

The application protocol
------------------------

- For aid C,and stream id S, `stream_app_data(C,S)`
  represents the stream data transmitted by the application to endpoint E
  on stread id S of cid C. This data may or may not have been transmitted in
  any QUIC packet.

- For aid C,and stream id S, `stream_app_end(C,S)` represents the
  length of stream data transmitted by the application to endpoint E
  on stread id S of cid C.

- For each aid C,and stream id S, `stream_app_pos(C,S)`
  represents the read position of the stream data transmitted by the application
  on stream id S to C. This is the number of bytes in the stream that have
  been read by the application.


```
function stream_app_data(C:cid,S:stream_id) : stream_data
function stream_app_data_end(C:cid,S:stream_id) : stream_pos
function stream_app_pos(C:cid,S:stream_id) : stream_pos


```
Initial state
-------------

The history variables are initialized as follows.  Initially, no
connections have been seen and no packets have been sent or
acknowledged.

```
after init {
    conn_seen(C) := false;
    last_pkt_num(C,L) := 0;
    conn_closed(C) := false;
    sent_pkt(C,L,N) := false;
    acked_pkt(C,L,N) := false;
    max_acked(C,L) := 0;
    ack_credit(C) := 0;
    trans_params_set(C:cid) := false;
    stream_seen(C,S) := false;
    stream_length(C,S) := 0;
    max_stream_data_set(C,S) := false;
    stream_finished(C,S) := false;
    stream_reset(C,S) := false;
    stream_app_pos(C,S) := 0;
    queued_hello(C) := false;
    queued_non_probing(C) := false;
    conn_enc_level(E,C) := encryption_level.initial;
    established_1rtt_keys(C:cid) := false;
    crypto_handler_pos(C,L) := 0;
    is_client(C) := false;
    conn_requested(S,D,C) := false;
    path_challenge_pending(C,D) := false;
    hi_non_probing(C) := 0;
    hi_non_probing_endpoint(C,E) := false;
}

```
Ghost events
------------

A ghost event is an internal event of the system under test that may
not be directly observable by the tester. These events may
correspond to communication between an application and a protocol
imlemenation, or an action within the application. In this case the
ghost event typically represents in some way the semantics of a
service being provided. Ghost evenst also sometimes correspond to
non-deterministic events within the implementation of a service, for
example the ordering of requests.

During testing, inference of unobservable ghost events may be
computationally intractable. In these cases, the system under test
may need to be instrumented to record these events. In other cases,
it may be feasible to infer the events from the observable
communication. This is usually done by instrumenting the
communication monitors with additional code the generates the ghost
events. Incorrect inference of ghost events may result in a false
alarm in testing.


Open event
==========

The first event in the life of a connection at a given is an `open_event`.
At this point, the endpoint chooses its default transport parameters. Once
the `open_event` has occurred, the security handshake can begin.

Requirements:

- The connection must not already be open at the endpoint
- The transport parameters have not already been set


before open_event(src:ip.endpoint, dst:ip.endpoint, pcid:cid, tp:trans_params_struct) {
    require conn_open(src,pcid);
    require ~trans_params_set(dst,pcid)
    conn_open(src,pcid) := true;
    trans_params_set(dst,pcid) := true;
    trans_params(dst,pcid) := tp;
}    

Application send event
======================

This event corresponds to transfer of application data to QUIC to
be transmitted.

Requirements

- None

Effects

- The data bytes are appended to `stream_app_data(C,S)` where `C` is
  the cid and `S` is the stream id.


```
action app_send_event(src:ip.endpoint, dst:ip.endpoint, dcid:cid, s : stream_id, data : stream_data)

after app_send_event {
    var idx := data.begin;
    while idx < data.end {
	stream_app_data(dcid,s) := stream_app_data(dcid,s).append(data.value(idx));
        stream_app_data_end(dcid,s) := stream_app_data(dcid,s).end;
	idx := idx.next
    }
}

```
Encryption level change

```
action set_encryption_level(src:ip.endpoint, scid:cid, e:encryption_level)

after set_encryption_level {
    conn_enc_level(src,scid) := e;
}

```
TLS send event
==============

This event corresponds to transmission of data between from a TLS
endpoint to its peer on a given connection. In concept it occurs at
the moment when the implmentation of TLS passes data to QUIC to be
transmitted to the peer. This is a ghost event, since it is not
visible on the wire. However, it can easily be inferred by examining
the QUIC crypto frames.

Requirements

- The connection must be open [3].
- The data must consist of a sequence of whole TLS records [1]

Effects

- The TLS data is appended to the crypto data [4].

- The effect of each transmitted TLS record on the QUIC connection
  state is defined by `handle_tls_record`, below [2]. A TLS
  extension to client and server hello messages carries the
  connection's initial transport parameters and is required.


```
before tls_send_event(src:ip.endpoint, dst:ip.endpoint, scid:cid, dcid:cid, data : stream_data,
                      pos : stream_pos, e:encryption_level) {
    var jdx := data.begin;
    var end := pos + data.end;
    if end > crypto_data(scid,e).end {
        crypto_data(scid,e) := crypto_data(scid,e).resize(end,0);
    };
    while jdx < data.end {
        var cpos := pos+jdx;
        var byte := data.value(jdx);
        require crypto_data_present(scid,e,cpos) -> crypto_data(scid,e).value(cpos) = byte;
        crypto_data_end(scid,e) := end;
	crypto_data(scid,e) := crypto_data(scid,e).set(cpos,byte);  # [4]
        crypto_data_present(scid,e,cpos) := true;
	jdx := jdx.next
    };
    crypto_data_end(scid,e) := crypto_data(scid,e).end;
    var max_present : stream_pos := 0;
    while max_present < crypto_data_end(scid,e) & crypto_data_present(scid,e,max_present) {
        max_present := max_present.next
    };
```
require conn_open(src,dcid);  # [3]
```
    var avail := crypto_data(scid,e).segment(crypto_handler_pos(scid,e),max_present);
    var res := tls.handshake_parser.deserialize(avail,0);
    var hs := res.value;
    call tls.handshake_data_event(src,dst,avail.segment(0,res.pos));
    crypto_handler_pos(scid,e) := crypto_handler_pos(scid,e) + res.pos;
    var idx := hs.begin;
    while idx < hs.end {
        var h := hs.value(idx);
        call tls.handshake_event(src,dst,h);
        call handle_tls_handshake(src,dst,scid,dcid,h);  #[2]
        idx := idx.next
    };
}

```

TLS receive event
=================

This event corresponds to transfer of data from QUIC to the TLS
implementation. 

```
action tls_recv_event(src:ip.endpoint, dst:ip.endpoint, scid:cid, dcid:cid, e:encryption_level, lo: stream_pos, hi : stream_pos)

around tls_recv_event {

    require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
    require lo < hi & lo = crypto_pos(dcid,e) & hi <= crypto_length(scid,e);
```
   require (data.end) + crypto_pos(pcid,0) <= crypto_length(pcid,0);
   require 0 <= N & N < data.end -> data.value(N) = crypto_data(pcid,0).value(N + crypto_pos(pcid,0));
```
    ...

    crypto_pos(dcid,e) := hi;
```
   crypto_pos(pcid,0) := crypto_pos(pcid,0) + data.end 
```
}

```

Client initial request event
============================

This event occurs when a client transmits in initial request to open
a connection.

```
action client_initial_request(src:ip.endpoint,dst:ip.endpoint,pkt:quic_packet)

```
Packet events
-------------

A packet event represents the transmision of a UDP packet `pkt` from
QUIC source endpoint `src` to a QUIC destination endpoint `dst`
containing a sequence of queued frames.

The packet *kind* depends on the field `hdr_type` according to
the following table:

  | hdr_type  | kind      |
  |-----------|-----------|
  | 0x7f      | Initial   |
  | 0x7d      | Handshake |


### Requirements

- The packet payload may not be empty [7].

- The header type must be one of the above [8].

- An Initial packet represents an attempt by a client to establish a
  connection. The scid is arbitrary, but must not have been previously
  seen. The dcid is unspecified. The initial packet must consist (apart from padding) of a
  single crypto, containing the initial security
  handshake information [1].

- A Handshake packet is sent in response to an Initial packet or
  a previous Handshake. In the latter case, the dcid must match
  the scid provided by the peer.

- Initial packets may not be sent on a cid that has
  been closed by the sender [6].

- Once a cid has been renamed by the server, it may no longer be used in a packet header.
  Instead the new cid must be used. [9]

- A sender may not re-use a packet number on a given scid [4].

- A packet containing only ack frames and padding is *ack-only*.
  For a given cid, the number of ack-only packets sent from src to dst
  must not be greater than the number of non-ack-only packets sent
  from dst to src [5].

- For a given connecitn, a server must only send packets to an
  address that at one time in the past sent the as packet with
  the highest packet numer thus far received. See notes below on
  migration and path challenge. [10]

### Effects

- The `conn_seen` and `sent_pkt` relations are updated to reflect
  the observed packet [1].
- The `last_pkt_num` function is updated to indicate the observed
  packets as most recent for the packet's source and cid.
  

### Notes

- The effective packet number is computed according to the procedure
  `decode_packet_number` defined below.

- It isn't clear whether a packet that is multiply-delivered packet
  can be responded to by multple ack-only packets. Here, we assume it
  cannot. That is, only a new distinct packet number allows an ack-only
  packet to be sent in response.

- On see a packet form a new address with the highest packect number
  see thus far, the server detects migration of the client. It
  begins sending packets to this address and initiates path
  validation for this address. Until path validation succeeds, the
  server limits data sent to the new address. Currently we cannot specify
  this limit because we don't know the byte size of packets or the timings
  of packets. On detecting migration, the server abandons any pending path
  validation of the old address. We don't specify this because the definition
  of abandonment is not clear. In practice, we do observe path challenge frames
  sent to old addresses, perhaps because these were previously queued. QUESTION:
  abandoning an old path challenge could result in an attacker denying the ability
  of the client to migrate by replaying packets, spoofing the old address.
  The server would alternate between the old (bogus) and new address, and thus
  never complete the path challenge of the new address. 



```
around packet_event(src:ip.endpoint,dst:ip.endpoint,pkt:quic_packet) {

```
TEMPORARY
```
    require ~pkt.hdr_long -> (pkt.dcid = 5 & pkt.scid = 0);
    require pkt.hdr_long -> (pkt.dcid > 0 & pkt.scid > 0);
    require pkt.hdr_token_length = pkt.hdr_token.end;
    require pkt.hdr_token_length ~= 0 -> (pkt.hdr_long & pkt.hdr_type = 0x7f);

```
Allowed header types

```
    require pkt.hdr_long -> (pkt.hdr_type = 0x7d | pkt.hdr_type = 0x7f);
    require ~pkt.hdr_long -> pkt.hdr_type = 0x30;

```
Extract the source and destination cid's and packet number from the packet.

```
    var dcid := pkt.dst_cid;
    var scid := pkt.hdr_cid;

```
On long headers, both cids are given. Record the correspondence.
If the scid has been mapped, make sure it maps to the dcid.

```
    if pkt.hdr_long {
        require cid_dst_to_src_set(scid) -> cid_dst_to_src(scid) = dcid
    }

```
On short headers, the scid is not given, so we use the recorded value

TODO: this is not reliable for testing, since a short header might
arrive before a corresping long header due to packet re-ordering. We
need to handle this case. This is a general problem with the test setup:
the specification applies to packets in the actual order of transmission,
while the tester may receive them out-of-order. One fix for this would be
to have the spec also allow re-ordering, but this might introduce some
significant complexity.

```
    else {
        require cid_dst_to_src_set(dcid);
        scid := cid_dst_to_src(dcid)
    };

    var e := queued_level(scid);

```
The header type depends on the encryption level

```
    require e = encryption_level.initial -> pkt.hdr_long & pkt.hdr_type = 0x7f;
    require e = encryption_level.handshake -> pkt.hdr_long & pkt.hdr_type = 0x7d;
    require e = encryption_level.other -> ~pkt.hdr_long;

    var pnum := decode_packet_number(src,scid,e,pkt.hdr_pkt_num);

    call show_pstats(scid,e,pnum);

    require ~sent_pkt(scid,e,pnum);  # [4]

```
The payload may not be empty

```
    require num_queued_frames(scid) > 0;  # [7]

```
A packet is an initial packet iff it contains a client hello.

TODO: what to say about this?
assert pkt.hdr_type = 0x7f <-> queued_hello(scid);

Record that the connection has been seen from this source, and
the packet has been sent.

```
    sent_pkt(scid,e,pnum) := true;  # [1]

```
Record the packet number as latest seen

```
    last_pkt_num(scid,e) := pnum;


```
The payload must exactly match the queued frames.

```
    require pkt.payload = queued_frames(scid);

```
var idx : frame.idx := 0;
while idx < pkt.payload.end {
    call pkt.payload.value(idx).handle(src,dst,dcid);
    idx := idx + 1
}

TEMPORARY: don't allow client migration during handshake

```
    require conn_seen(scid) & pkt.hdr_long & is_client(scid) -> conn_requested(src,dst,scid);

```
Packet must be sent to the endpoint from which the highest numbered
packet has been received.

```
    require conn_seen(dcid) -> hi_non_probing_endpoint(dcid,dst);  # [10]

```
If this is a non-probing packet, update the highest non-probing
packet number seen on from this aid.
QUESTION: what if two differenht paths send the same packet number?
QUESTION: how do you compare packet numbers with different encryption levels?

```
    if queued_non_probing(scid) {
        if e = encryption_level.other {
            if pnum >= hi_non_probing(scid) {
                hi_non_probing(scid) := pnum;
                hi_non_probing_endpoint(scid,src) := true;
            }
        } else {
            hi_non_probing_endpoint(scid,src) := true;
        }
    }

    ...

```
TEMPORARY: The following are repeated because currently locals defined in
the "before" section cannot be accessed in the "after" section.

```
    var dcid := pkt.dst_cid;
    var scid := pkt.hdr_cid;
    if ~pkt.hdr_long {
        scid := cid_dst_to_src(dcid)
    };

    if pkt.hdr_type = 0x7f {
        require forall (I:frame.idx) 0 <= I & I < pkt.payload.end ->
        ((pkt.payload.value(I) isa frame.ack)
        | (pkt.payload.value(I) isa frame.crypto)
        | (pkt.payload.value(I) isa frame.rst_stream)
        | (pkt.payload.value(I) isa frame.connection_close));
```
require ~conn_closed(src,scid);  # [6]

An initial packet with an unseen destination cid is a connection request.
```
        if ~conn_seen(dcid) {
            call client_initial_request(src,dst,pkt);
            conn_requested(src,dst,scid) := true;
        };

    };

    conn_seen(scid) := true;  # [1]

```
An ack-only packet must be in response to a non-ack-only packet

```
    var ack_only := forall (I:frame.idx) 0 <= I & I < pkt.payload.end ->
                                 (pkt.payload.value(I) isa frame.ack);
    if ack_only {
```
	require ack_credit(scid) > 0;  # [5]
```
	ack_credit(scid) := ack_credit(scid) - 1;
    } else {
	ack_credit(dcid) := ack_credit(dcid) + 1;
    };

```
The queued frames are deleted

```
    queued_frames(scid) := frame.arr.empty;
    queued_hello(scid) := false;
    queued_non_probing(scid) := false;
    num_queued_frames(scid) := 0;
}

```
The frame protocol
==================

The frame protocol is defined by a sequence of frame events.
This protocol is layered on the packet protocol, such that
each packet event contains a sub-sequence of the frame events.

The frame events are subdivided into variants called frame types.
For each frame type, we define a ghost event `handle` corresponding
to the generation of a frame and its transfer to the packet protocol
for transmission. Frame events effect the protocol state by
enqueueing frames to be encapsulated into packets. The effect of
this is that frame and packet events are interleaved, such that the
frames in each packet occur immediately before the packet event in
the same order in which they occur in the packet payload. TODO:
While this ordering seems sensible from a semantic point of view,
implementations might transmit frames out of order. Requiring ghost
frame events to be in order might complicate a modular proof of the
implementation.

Each frame has an encryption level. The enryption level determines
the keys used to protect to protect that packet payload. Only frames
of the same encryption level may be encapsulated in the same packet
(however, multiple packets may be concatenated in a single UDP
datagram). This requirement is enforced by requiring that every
frame queue contains only frames of the same encryption level. The
frame handler for each type enforces this condition.


This generic `handle` action for frames is specialized for each
frame type.  Its arguments are the frame, the source and destination
endpoints, the source and destination cids and the encryption level.


TODO: we assume here that a frame can only be sent at a given
encryption level if the keys for that level have already been
established.  For 1rtt frames this means that a TLS finish message
must have bee sent in some prior frame. This is helpful to prevent
the peer from dropping packets in tests, but not realistic, since
packet re-ordering could cause the 1rtt frame to be received before
the required handshake message.  In principle, we should allow this
case, but reduce its probability in testing.

```
object frame = {
   ...
    action handle(f:this,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level) = {
	require false; # this generic action should never be called
    }
}

```
Frame events cause frames to be enqueued for transmission in a
packet. This action enqueues a frame.

Effects:

- Appends frame to the frame queue for the given source endpoint and cid.
- Updates auxiliary functions `num_queued_frames` and `queued_level`.

Note: the auxilary functions contain redundant information that is useful for
specifying packet events. By encoding history information in this way, we make
it easier for constraint solvers to construct tests.

```
action enqueue_frame(src:ip.endpoint, scid:cid, f:frame, e:encryption_level, probing:bool) = {
    queued_frames(scid) := queued_frames(scid).append(f);
    num_queued_frames(scid) := queued_frames(scid).end;
    queued_level(scid) := e;
    if ~probing {
        queued_non_probing(scid) := true;
    }

}

```
#### Ack handler

The set of packet numbers acknowledged by an Ack frame is determined
by the `largest_ack` field and the `ack_blocks` field. Each Ack
block acknowledges packet numbers in the inclusive range `[last - gap, last -
gap - blocks]` where `gap` and `blocks` are the fields of the Ack
block and `last` is `largest_ack` minus the sum of `gap + blocks`
for all the previous ack blocks.

The `gap` field for the first ack block is always zero and is not
present in the low-level syntax.

Requirements:

- Every acknowledged packet must have been sent by the destination endpoint [1].
- Keys must be established for the given encryption level [4].

Effects:

- The acknowledged packets are recorded in the relation `acked_pkt(C,N)`
  where `C` is the *source* of the acknowledged packet (not of the Ack)
  and `N` is the packet number [2].
- The greatest acked packet is also tracked in `max_acked(C,e)` [3]

TEMPORARY: use this to enforce new acks in testing

```
var force_new_ack : bool

object frame = {
    ...
    object ack = {
        ...
        action handle(f:frame.ack,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)

	around handle {
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other -> established_1rtt_keys(scid);  # [4]
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            var idx : frame.ack.block.idx := 0;
            var last := f.largest_acked;
            if max_acked(dcid,e) < last {
                max_acked(dcid,e) := last;  # [3]
            };
            require f.ack_blocks.end > 0;
            var some_new_ack := false;
            while idx < f.ack_blocks.end {
                var ack_block := f.ack_blocks.value(idx);
                require idx > 0 -> ack_block.gap < last - 1;
                var upper := last - ((ack_block.gap+2) if idx > 0 else 0);
                require ack_block.blocks <= upper;
                last := upper - ack_block.blocks;
		var jdx := last;
		while jdx <= upper {
                    require sent_pkt(dcid,e,jdx);  # [1]
                    if ~acked_pkt(dcid,e,jdx) {
                        some_new_ack := true;
                    };
		    acked_pkt(dcid,e,jdx) := true;
		    jdx := jdx + 1
		};
```
               acked_pkt(dcid,N) := (last <= N & N <= upper) | acked_pkt(dcid,N);  # [2]
```
                idx := idx.next;
            };
            if _generating {
                require some_new_ack;
            }
	    ...
            force_new_ack := false;
            call enqueue_frame(src,scid,f,e,false);
        }
    }
}

```
Stream frames carry stream data. 

Requirements:

- The upper bound of the stream frame may not exceed the current
  value of `max_stream_data_val` for the given destination and cid, if
  it has been set [2].

- If the stream is finished, the the frame offset plus length must
  not exceed the stream length [5].

- The stream id must be less than or equal to
  the max stream id for the kind of the stream [6].

- The stream must not have been reset [7].

- The connection must not have been closed by the source endpoint [8].

- The connection id must have been seen at the source [9]
  and the connection between source and destination must not be initializing [10].

- The 1rtt keys have been established [11].

Effects:

- If the stream has not been seen before, and if the
  `initial_max_stream_data` transport parameter has been set, then
  the `max_stream_data_val` value is set to the value of the
  `initial_max_stream_data` transport parameter [3].

- The length of the stream is updated. 

- If the fin bit is set, the stream is marked as finished.

Question: what is the value of `initial_max_stream_data` for stream
zero? Here we assume there is no back-pressure on stream zero.

Question: is the FIN bit allowed on stream zero?

```
object frame = {
    ...
    object stream = {
        ...
        action handle(f:frame.stream,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)

	around handle {
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other & established_1rtt_keys(scid);  # [11]
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            require ~conn_closed(scid);  # [8]

            var offset := f.offset if f.off else stream_length(dcid,f.id);
	    require ((offset) + (f.length)) <= stream_app_data_end(dcid,f.id);
            call show_offset_length(offset,f.length);
	    require f.data = stream_app_data(dcid,f.id).segment(offset,offset+f.length); 

            var kind := get_stream_kind(f.id);
            require max_stream_set(dcid,kind) -> f.id < max_stream(dcid,kind);  # [6]
```
require ~stream_reset(dcid,f.id);  # [7]
require conn_seen(scid);  # [9]
if ~stream_seen(dcid,f.id) {
    if initial_max_stream_data.is_set(trans_params(dcid)) {
        max_stream_data_val(dcid,f.id) :=
            initial_max_stream_data.value(trans_params(dcid)).stream_pos_32;  # [3]
        max_stream_data_set(dcid,f.id) := true
    };
    if initial_max_stream_id_bidi.is_set(trans_params(dcid)) {
        max_stream(dcid,bidir) :=
            (initial_max_stream_id_bidi.value(trans_params(dcid)).stream_id_16) * 4;  # [3]
        max_stream_set(dcid,bidir) := true
    };
    if initial_max_stream_id_uni.is_set(trans_params(dcid)) {
        max_stream(dcid,unidir) :=
            (initial_max_stream_id_uni.value(trans_params(dcid)).stream_id_16) * 4;  # [3]
        max_stream_set(dcid,unidir) := true
    };
    stream_length(dcid,f.id) := 0;
};
if max_stream_data_set(dcid,f.id) {
    require ((offset) + (f.length)) <= max_stream_data_val(dcid,f.id)  # [2]
};
```
            stream_seen(dcid,f.id) := true;
	    ...
            var offset := f.offset if f.off else stream_length(dcid,f.id);
            var length := offset + f.length;
```
require stream_finished(dcid,f.id) -> length <= stream_length(dcid,f.id);  # [5]
```
            if stream_length(dcid,f.id) < length {
                stream_length(dcid,f.id) := length
            };
            if f.fin {
                stream_finished(dcid,f.id) := true;
            };
            call enqueue_frame(src,scid,f,e,false);
        }
    }
}

```
Crypto frames carry crypto handshake data, that is, TLS records.

Requirements:

- The connection must not have bee closed by the source endpoint [1].

Effects:

- The length of the crypto stream is updated. 

```
object frame = {
    ...
    object crypto = {
        ...
        action handle(f:frame.crypto,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)

	around handle {
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            call show_enc_level(e);
            require ~conn_closed(scid);  # [1]

	    require ((f.offset) + (f.length)) <= crypto_data_end(scid,e);
	    require f.data = crypto_data(scid,e).segment(f.offset,f.offset+f.length); 

	    ...
            var length := f.offset + f.length;
            if crypto_length(scid,e) < length {
                crypto_length(scid,e) := length
            };
            var idx := f.offset;
            while idx < f.offset + f.length {
                crypto_data_present(scid,e,idx) := true;
                idx := idx.next
            };
            call enqueue_frame(src,scid,f,e,false);
            if e = encryption_level.handshake {
                established_1rtt_keys(scid) := true;
            }
        }
    }
}

```
Reset stream frames cause an identified stream to be abruptly terminated,
meaning the no further transmissions (or retransmissions) will be sent for
this stream and the receiver may ignore any data previously transmitted.

Requirements:

- Stream id must not exceed maximim stream id for the stream kind [4].
- QUESTION: Can a previously reset stream be reset?
- The final stream position may not be lesser than that of any previous
  stream frame for the same stream id [1].
- The connection must not have been closed by the source endpoint [5].
- The encryption level must be 0rtt or 1rtt [6].
- If stream was previously reset or finished, final offset must be same [7].

Effects:

- The specified stream id is marked as reset [2].
- The stream length is set to the given final offset [3].

Question: Where is it written that reset stream frames cannot occur in
initial or handshake packets?

```
object frame = { ...
    object rst_stream = { ...
        action handle(f:frame.rst_stream,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)

	around handle {
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other & established_1rtt_keys(scid);  # [6]
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            require ~conn_closed(scid);  # [5]
            require cid_dst_to_src_set(scid) & cid_dst_to_src(scid) = dcid;
            require stream_length(dcid,f.id) <= f.final_offset;  # [1]
            var kind := get_stream_kind(f.id);
            require max_stream_set(dcid,kind) -> f.id <= max_stream(dcid,kind);  # [4]
            require (stream_reset(dcid,f.id) | stream_finished(dcid,f.id)) -> stream_length(dcid,f.id) = f.final_offset;
            stream_reset(dcid,f.id) := true;  # [2]
            stream_length(dcid,f.id) := f.final_offset;  #[3]
	    ...
            call enqueue_frame(src,scid,f,e,false);
        }
    }
}

```
Max stream id frames cause the maximum stream id to be set. 
The receiver of the max stream id may use stream ids up to and including
the given maximum. Bit 1 of the stream id (the next-to-least significant)
determines whether the limit is set for unidirectional or bidirectional
streams. A max stream id containing a stream id lower than the current
maximum is allowed and ignored.

Requirements:

- The connection must not have been closed by the source endpoint [2].
- Max stream id frames may not occur in initial or handshake packets [3].
- The role of the stream id must equal the role of the peer in the given connection. [4]
  QUESTION: this requirement is not stated in the draft spec, but it is enforced
  by picoquic (see anomaly6). The spec should state explicitly what happens in this case.

Effects:

- The maximum stream id is set [1].

QUESTION: must the stream id's be less than the max or less than or equal?
Picoquic seems to think less than, but the is not clear in the draft.

```
object frame = { ...
    object max_stream_id = { ...
        action handle(f:frame.max_stream_id,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)

	around handle {
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other & established_1rtt_keys(scid);  # [3]
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            require cid_dst_to_src_set(scid) & cid_dst_to_src(scid) = dcid;
            require ~conn_closed(scid);  # [2]
            require get_stream_role(f.id) = role.server <-> is_client(scid);  # [4]

            var kind := get_stream_kind(f.id);
            if ~ (max_stream_set(dcid,kind) & f.id < max_stream(dcid,kind)) {
                max_stream_set(dcid,kind) := true;
                max_stream(dcid,kind) := f.id; #  [1]
            }
	    ...
            call enqueue_frame(src,scid,f,e,false);
        }
    }
}

```
Connection close frames indicate to the peer that the connection is being closed.
It is unclear what this means operationally, but it seems reasonable to assume that the
endpoint closing the connection will not send or receive any further data on the connection,
so it is as if all the open streams are reset by this operation.

A connection close frame can occur at any time. 

Questions:

- Are Ack frames still allowed after connection close?
- Are retransmissions allowed after connection close?
- When is a connection close allowed?

Requirements:

- The source and destination cid's must be connected. In effect,
  this means that a server hello message must have been sent for
  this connection Therefore a client cannot send a connection close
  before receiving at least one handshake message from the
  server. QUESTION: the spec is a bit vague about this, stating
  "Handshake packets MAY contain CONNECTION_CLOSE frames if the
  handshake is unsuccessful." Does "unsuccessful" necessarily mean that
  some handshake has been received? Also, can an initial packet contain
  connection close? 

Effects:

- The connection state is set to closed for the source endpoint.


```
object frame = { ...
    object connection_close = { ...
        action handle(f:frame.connection_close,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)
	around handle{
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other -> established_1rtt_keys(scid);
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            require cid_dst_to_src_set(scid) & cid_dst_to_src(scid) = dcid;
            require f.reason_phrase_length = f.reason_phrase.end;
            conn_closed(scid) := true;
	    ...
            call enqueue_frame(src,scid,f,e,false);
        }
    }
}

```
Application close frames indicate to the peer that the connection is being closed.
It is unclear what this means operationally, but it seems reasonable to assume that the
endpoint closing the connection will not send or receive any further data on the connection,
so it is as if all the open streams are reset by this operation.

An application close frame can occur at any time. 

Questions:

- Are Ack frames still allowed after application close?
- Are retransmissions allowed after application close?
- When is a application close allowed?

Requirements:

(None)

Effects:

- The connection state is set to closed for the source endpoint.


```
object frame = { ...
    object application_close = { ...
        action handle(f:frame.application_close,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)
	around handle{
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other -> established_1rtt_keys(scid);
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            conn_closed(scid) := true;
	    ...
            call enqueue_frame(src,scid,f,e,false);
        }
    }
}

```
Max stream data frames set the limit on data bytes that the source endpoint is willing
to receive for a given stream.

Requirements

- The stream must be open for receiving at the source endpoint [1].

Effects
- If the given limit is greater than any previously set limit, then
  the max stream data limit for the given stream is updated [2].


```
object frame = { ...
    object max_stream_data = { ...
        action handle(f:frame.max_stream_data,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)
	around handle {
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other & established_1rtt_keys(scid);
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);

            if ~(max_stream_data_set(dcid,f.id) & f.pos < max_stream_data_val(dcid,f.id)) {
                max_stream_data_set(dcid,f.id) := true;
                max_stream_data_val(dcid,f.id) := f.pos;  # [2]
            }
	    ...
            call enqueue_frame(src,scid,f,e,false);
        }
    }
}

```
Ping frames contain no data and have no semantics. They can
be used to keep a connection alive.

```
object frame = { ...
    object ping = { ...
        action handle(f:frame.ping,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)
	around handle {
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other -> established_1rtt_keys(scid);
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            ...
            call enqueue_frame(src,scid,f,e,false);
        }
    }
}

```
New connection if frames are used to transmit additional cid's to the peer.

```
object frame = { ...
    object new_connection_id = { ...
        action handle(f:frame.new_connection_id,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)
	around handle {
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other & established_1rtt_keys(scid);
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            ...
            call enqueue_frame(src,scid,f,e,true);
        }
    }
}

```
Path challenge frames are used to request verification of ownership
of an endpoint by a peer.

```
object frame = { ...
    object path_challenge = { ...
        action handle(f:frame.path_challenge,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)
	around handle {
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other & established_1rtt_keys(scid);
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            require f.data.end = 8;
            ...
            path_challenge_pending(dcid,f.data) := true;
            call enqueue_frame(src,scid,f,e,true);
        }
    }
}

```
Path response frames are used to verify ownership of an endpoint in
response to a path_challenge frame.

```
object frame = { ...
    object path_response = { ...
        action handle(f:frame.path_response,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)
	around handle {
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other & established_1rtt_keys(scid);
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            require f.data.end = 8;
            require path_challenge_pending(dcid,f.data);
            ...
            path_challenge_pending(scid,f.data) := false;
            call enqueue_frame(dst,dcid,f,e,false);
        }
    }
}

```
New token frames are sent by the server to provide the client a
token for establishing a new connection.

```
object frame = { ...
    object new_token = { ...
        action handle(f:frame.new_token,src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,e:encryption_level)
	around handle {
            require cid_dst_to_src_set(dcid) & cid_dst_to_src(dcid) = scid;
            require e = encryption_level.other & established_1rtt_keys(scid);
            require num_queued_frames(scid) > 0 -> e = queued_level(scid);
            require ~is_client(scid);
            ...
            call enqueue_frame(dst,dcid,f,e,false);
        }
    }
}

```
### Packet number decoding

The packet number is decoded from the packet header fields as follows.

If the connection is new, the field `hdr_pkt_num` gives the
exact first packet number. Otherwise, it represents only a number
of low order bits. The high-order bits must be inferred from the
last packet number seen for this connection.

For short format packets. the number of low order bits present
in `hdr_pkt_num` depends on the `hdr_type` field of the packet,
according to this table:

  | hdr_type | bits |
  |----------|------|
  | 0x1d     | 32   |
  | 0x1e     | 16   | 
  | 0x1f     |  8   |

For long format packets, the number of bits is always 32.  The
decoded packet number is the nearest number to the last packet number seen
whose whose low-order bits agree with `hdr_pkt_num`. Note this is ambiguous
in the case that `hdr_pkt_num - last_pkt_num = 2^(n-1) mod 2^n` where `n`
is the number of bits, since `last_pkt_num + 2^(n-1)` and `last_pkt_num - 2^(n-1)`
both equal `hdr_pkt_num` modulo `2^n` and are equidistant from `last_pkt_num`.
This case is forbidden.

Requirements

- The sent packet number must be no greater than `la + max/2` where
  `la` is the greatest acknowledged packet number (or zero if there
  have been no acks) and `max` is a largest number that can be
  represented with the number of bits provided [1].

Notes:

- The IETF draft uses this langauge: "The sender MUST use a packet
  number size able to represent more than twice as large a range
  than the difference between the largest acknowledged packet and
  packet number being sent." The meaning of "more than twice as
  large a range" isn't clear, but here we take it to mean that
  `2 * (pnum - la) ` is representable. It is also not clear how the
  maximum packet number is computed if no acks have been received,
  but we assume here that `la` is zero in this case.

  TODO: this seems inconsistent with the following statement: "The
  initial value for packet number MUST be selected randomly from a
  range between 0 and 2^32 - 1025 (inclusive)." Possibly there is no
  upper limit on the packet number if no acks have been received
  yet, but this seems questionable.

```
action decode_packet_number(src:ip.endpoint,scid:cid,e:encryption_level,pnum:pkt_num) returns (pnum:pkt_num) = {

    var la := max_acked(scid,e);

```
This is the last number transmitted by the source on this connection.

```
    var last := last_pkt_num(scid,e);

```
TODO: for now, assume always 30-bit packet number format

```
    if true {
        var diff : pkt_num := bfe[0][29](pnum - last);
        pnum := last + diff;
        if diff >= 0x20000000 {
            pnum := pnum - 0x40000000
        };
```
require pnum <= la + 0x7ffffffe;
TEMPORARY: work around minquic bug
```
        require pnum <= last + 0x7;
    }
}

```
TLS extensions are used in the client hello and server hello
messages to carry the QUIC transport parameters, via special TLS
extension type `quic_transport_parameters`. This type is define in
the reference `quic_transport_parameters`. Here we define the
protocol rules associated with `quic_transport_parameters`.

The `handle_tls_handshake` rules apply to each `client_hello` or
`server_hello` message, in order of occurrence in the crypto data.
The last parameter, `is_client_hello`, is true for a `client_hello`
message (see below).

A server hello establishes the connection between the server and the
client cids [1]. We have to do it this way because there is no way
looking at just the QUIC packet header and frame types to
distinguish a client initial message from a server initial message.

We also infer changes in encryption level from the handshake message
types. In particular, any non-hello message moves us from the initial
level to the handshake level. TODO: this might not be reliable if there
are changes to TLS. It would be better to get an explicit signal from
TLS that the level is changing. TODO: Does the QUIC-TLS specification
say explicitly when the encryption level should change?


```
action handle_tls_handshake(src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid,hs:tls.handshake) = {

    if some(ch:tls.client_hello) hs *> ch {
        queued_hello(scid) := true;
        is_client(scid) := true;
        call handle_tls_extensions(src,dst,scid,ch.extensions,true);
    }
    else if some(sh:tls.server_hello) hs *> sh {
        queued_hello(scid) := true;
```
       call map_cids(src,scid,dcid);    # [1]
       call map_cids(dst,dcid,scid);
```
        call handle_tls_extensions(src,dst,scid,sh.extensions,false);
    }
}

action map_cids(dcid:cid,scid:cid) = {
    cid_dst_to_src_set(dcid) := true;
    cid_dst_to_src(dcid) := scid
}

```
An open connection event occurs on server src when it recieves a connection
request from a client dst, with cid dcid. The server selects its own cid scid,
and registers the peer relationship between the two cids.

Requirements:

- A connection must have been requested [1].
- The two cids must not be previously associated to any peer [2]. 

```
action open_connection(src:ip.endpoint,dst:ip.endpoint,scid:cid,dcid:cid)

around open_connection {
    require conn_requested(dst,src,dcid);  # [1] 
    require ~cid_dst_to_src_set(dcid) & ~cid_dst_to_src_set(scid);  # [2]
    ...
    call map_cids(scid,dcid);
    call map_cids(dcid,scid);
}

```
Requirements:

1. Transport parameters must be declared in the client hello
   and server hello message [1] and may be declared only once [2].

The rules in `handle_client_transport_parameters` apply to each
`quic_transport_parameters` extension instance in order of
occurrence.


```
action handle_tls_extensions
    (src:ip.endpoint,
     dst:ip.endpoint,
     scid:cid,
     exts:vector[tls.extension],
     is_client_hello:bool) =
{

```
We process the extensions in a message in order.

```
    var idx := exts.begin;
    while idx < exts.end {
        var ext := exts.value(idx);

```
For every `quic_transport_parameters` extension...

```
        if some (tps:quic_transport_parameters) ext *> tps {
            call handle_client_transport_parameters(src,dst,scid,tps,is_client_hello);
            require ~trans_params_set(scid);  # [2]
            trans_params_set(scid) := true;

        };
        idx := idx.next
    };
```
TEMPORARY: exempt server from sending transport parameters, since
minq doesn't implement this.
TEMPORARY: skip this until Botan supports it
   if is_client_hello {
       require trans_params_set(scid);
   }
```
}


```
The rules in `handle_transport_parameter` apply to each
`transport_parameter` instance a `quic_transport_parameters`
extension, in order of occurrence.

Requirements:

- The endpoint must issue an `initial_max_stream_data` value [1].
- The endpoint must issue an `initial_max_data` value [2].
- The endpoint must issue an `idle_timeout` value [3].
- A client must not issue an `stateless_reset_token` value [4].

Note:

- Setting a transport parameter requires that the parameter is not
  previously set.

```
action handle_client_transport_parameters(src:ip.endpoint,dst:ip.endpoint,scid:cid,
                                          tps:quic_transport_parameters,
                                          is_client_hello : bool) =
{
    call client_transport_parameters_event(src,dst,scid,tps);
    var idx := tps.transport_parameters.begin;
    while idx < tps.transport_parameters.end {
        trans_params(scid) := tps.transport_parameters.value(idx).set(trans_params(scid));
        idx := idx.next
    };
    require initial_max_stream_data_bidi_local.is_set(trans_params(scid));  # [1]
    require initial_max_data.is_set(trans_params(scid));  # [2]
    require idle_timeout.is_set(trans_params(scid));  # [3]
    require initial_max_stream_data_bidi_remote.is_set(trans_params(scid));  # [1]
    require initial_max_stream_data_uni.is_set(trans_params(scid));  # [1]
    if is_client_hello {
        require ~stateless_reset_token.is_set(trans_params(scid));  # [4]
    }
}






action client_transport_parameters_event(src:ip.endpoint,dst:ip.endpoint,scid:cid,
                                         tps:quic_transport_parameters)

import client_transport_parameters_event

action show_enc_level(e:encryption_level)

import show_enc_level

import action show_offset_length(offset:stream_pos,length:stream_pos)

import action show_pstats(scid:cid,e:encryption_level,pnum:pkt_num)
```
