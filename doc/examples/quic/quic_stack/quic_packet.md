
```
include quic_types
include quic_frame

```
QUIC Packets
------------

This section defines the QUIC packet datatype. Packets are the basic
unit of communication between endpoints. A packet has a *type*,
associated cid, an optional protocol version, and a packet
number. The *payload* of the packet consists of a sequence of
*frames* (see frame.ivy). A packet may be encoded in either *long*
or *short* format.


TODO: not needed?
object quic_long_type = {
   type this = {initial,retry,handshake,zero_rtt_protected}
}

### Packet

The type `quic_packet` represents packet. 

The fields are:

- *hdr_long*: true if the packet has long format [1]
- *hdr_type*: the packet type bits [2]
- *hdr_cid*: the associated cid [3]
- *hdr_version*: the protocol version, or zero for short format
- *hdr_pkt_num*: the packet number
- *hdr_token_length*: the retry token length (see section 4.4.1)
- *hdr_token*: the retry token  (see section 4.4.1)
- *payload*: the payload, a sequence of frames

```
object quic_packet = {
    type this = struct {
        hdr_long : bool, # [1]
        hdr_type : type_bits, # [2]
        hdr_version : version, # [4]
        dcid : cid_length, 
        scid : cid_length, 
        dst_cid : cid,
        hdr_cid : cid, # [3]
        hdr_token_length : stream_pos,
        hdr_token : stream_data,
        payload_length : stream_pos,
        hdr_pkt_num : pkt_num, # [5]
        payload : frame.arr # [6]
    }

    instance idx : unbounded_sequence
    instance arr : array(idx,this)
}
```
