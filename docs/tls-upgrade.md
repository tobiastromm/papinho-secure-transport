<!-- SPDX-License-Identifier: MPL-2.0 -->
# TLS upgrade after consumer-owned plaintext

PST remains protocol-agnostic. A consumer may use an already-connected transport for
plaintext, consume the complete application-defined upgrade response, stop all raw
reads and writes, and then attach that same transport to a PST connection. This is the
foundation for STARTTLS-style mail upgrades and CONNECT-style TLS tunnels; SMTP, IMAP,
HTTP and proxy parsing remain consumer responsibilities.

Before attach acceptance the consumer owns the transport. After PST accepts transferred
ownership, PST owns exactly one close even if TLS negotiation fails. Failure never
returns ownership, reconnects, rolls back to plaintext or selects another provider.
Failure before acceptance leaves ownership and close responsibility with the consumer.

Only a clean boundary is supported. The consumer must account for every plaintext byte
through the end of the upgrade response and must not read TLS record bytes before
attach. `PRE_READ_TLS_BYTES_BEFORE_ATTACH` is not supported in this scope. The negative
fixture removes one byte from a TLS record before attach and confirms a bounded terminal
TLS failure with no plaintext fallback or resurrection.

After attach, the ordinary API 2.1 contract applies unchanged: provider-authoritative
incremental handshake, wait-set registration and finite blocking wait, caller-owned
partial-I/O remainders, authenticated peer-name/SNI policy, clean reciprocal shutdown,
and truncated abrupt closure. CONNECT authenticates the TLS origin named by
`expected_peer_name`; it does not authenticate that origin as the tunnel endpoint.

M6 fixtures use generic `PLAINTEXT_*` markers and a transparent byte relay. They prove
one accepted client socket, fragmented boundary responses, no reconnect, TLS 1.2 on all
three providers, TLS 1.3 on eligible OpenSSL, custom trust, mTLS on RetroZilla NSS,
encrypted echo and reciprocal shutdown. They do not implement an application protocol.

The host validation exercised STARTTLS-style TLS 1.2 with OpenSSL, CONNECT-style TLS
1.2 with Schannel, STARTTLS-style TLS 1.2 mTLS with RetroZilla NSS, and CONNECT-style
TLS 1.3 with OpenSSL. Each positive case transferred the already-used socket, completed
incremental wait-set progress, exchanged and verified 25 encrypted bytes, and completed
reciprocal shutdown. A STARTTLS-style abrupt close after that encrypted exchange was
classified as `TRUNCATED` without losing the delivered data. Bounded repetition ran 50
STARTTLS-style and 50 CONNECT-style real-TLS cycles with zero crashes, hangs,
double-closes, ownership mismatches, or plaintext fallbacks.

```text
LEGACYMAIL_STARTTLS_FOUNDATION=READY
BROWSER_CONNECT_TLS_FOUNDATION=READY
```

These markers certify the PST boundary only; they do not claim completion of Browser or
LegacyMail integration.
