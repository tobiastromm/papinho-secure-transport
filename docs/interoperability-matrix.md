<!-- SPDX-License-Identifier: MPL-2.0 -->

# Interoperability Matrix

## Current development status — API 2.1 / library 0.6 candidate

This document preserves the historical interoperability work while making the current cross-provider state explicit. `TESTED` means an execution is recorded, `SUPPORTED` is an implemented contract, `NOT TESTED` has no evidence, `UNSUPPORTED` is explicitly unavailable, and `NOT APPLICABLE` means the current target matrix cannot form that composition.

M9 completed the 0.6.0 development candidate's cross-provider scheduler/security/stress matrix. Final physical NT4, clean-machine/package and publication validation remain M10 work; this is not yet a published 0.6.0 release claim.

### M9 CLIENT × SERVER network matrix

All positive rows use the provider-neutral API 2.1/SPI 3.0 contract and preserve pre-binding capability filtering, encrypted I/O, provider-authoritative readiness, terminal no-resurrection and reciprocal shutdown.

| CLIENT | SERVER | Target/process relationship | TLS 1.2 | TLS 1.3 |
|---|---|---|---|---|
| OpenSSL | OpenSSL | x64, separate processes | PASS | PASS |
| OpenSSL | Schannel | x64, separate processes | PASS | NOT ELIGIBLE: Schannel SERVER capability |
| Schannel | OpenSSL | x64, separate processes | PASS | NOT ELIGIBLE: Schannel CLIENT capability |
| Schannel | Schannel | x64, separate processes | PASS | NOT ELIGIBLE: Schannel role capabilities |
| NSS | NSS | x86, separate processes | PASS | PASS |
| OpenSSL | NSS | x64 CLIENT to x86 SERVER over network | PASS | PASS |
| Schannel | NSS | x64 CLIENT to x86 SERVER over network | PASS | NOT ELIGIBLE: Schannel CLIENT capability |
| NSS | OpenSSL | x86 CLIENT to x64 SERVER over network | PASS | PASS |
| NSS | Schannel | x86 CLIENT to x64 SERVER over network | PASS | NOT ELIGIBLE: Schannel SERVER capability |

Therefore:

```text
TLS 1.2: 9/9 factual pairs PASS
TLS 1.3: 4/4 eligible pairs PASS
TLS 1.3: 5 Schannel-ineligible pairs rejected by capability before binding
```

No automatic protocol downgrade is inferred from an ineligible TLS 1.3 request.

### Scheduler interoperability

M1–M3 add a portable wait-set, borrowed external/native sources, finite blocking waits and cross-thread wake. M9 proves those pieces together across provider workloads:

- cross-provider wait-set and finite wait: PASS;
- cross-provider wake and wake/timeout race matrix: PASS;
- stable registration order and bounded enumeration: PASS;
- external listener/source + OpenSSL: PASS;
- external listener/source + Schannel: PASS;
- external listener/source + RetroZilla NSS: PASS;
- borrowed listener ownership preserved; PST never calls `accept`;
- raw socket readiness remains only a hint for PST connections; RetroZilla NSS `PR_Poll` remains authoritative;
- mandatory periodic polling: NO;
- sequential `N * timeout` waiting: NO.

### Backpressure and partial I/O

M4/M9 prove bounded partial read/write, cross-direction `NEED_*` states, hot/slow multi-connection scheduling and exact byte preservation. OpenSSL passed a 4 MiB backpressure transfer; Schannel and NSS passed forced 3-byte fragmentation. Deterministic tests also force 7-byte writes and 5-byte reads.

```text
DATA_DUPLICATION=0
DATA_LOSS=0
BUSY_LOOP_DETECTED=NO
```

PST does not promise send-all, read-until-full or application fairness. The consumer retains unsent bytes and owns scheduling policy.

### TLS after prior plaintext use

M6/M9 prove generic STARTTLS-style and CONNECT-style upgrades on the **same connected transport**. The consumer performs plaintext setup, stops exactly at the upgrade boundary, transfers ownership to PST, then TLS begins. PST does not reconnect and does not parse SMTP/IMAP/HTTP.

After ownership acceptance, a TLS failure does not return the socket to plaintext ownership. TLS bytes pre-read by the consumer before attach remain intentionally unsupported and fail closed rather than triggering recovery/fallback.

This makes the PST transport boundary suitable for PapinhoLegacyMail STARTTLS and PapinhoBrowser CONNECT-to-origin TLS without claiming those application protocols are implemented inside PST.

### Trust, authentication and SNI

CUSTOM_TRUST and SYSTEM_TRUST remain separate policies. M9 passes representative server authentication, mTLS, Expected Peer Name, clientAuth/serverAuth usage and negative trust/authentication cases according to each provider's advertised role-scoped capabilities.

API 2.1 separates SNI routing from Expected Peer Name authentication:

- OpenSSL CLIENT SNI control: FULL;
- Schannel CLIENT SNI control: FULL;
- RetroZilla NSS CLIENT SNI control: PARTIAL.

RetroZilla NSS retains the published `SSL_SetURL` behavior and is not patched to manufacture independent SNI parity. Unsupported `DISABLED`/`EXPLICIT != expected peer` combinations that require full control make NSS ineligible before binding.

### Provider selection

M9 reconfirms Combined Schannel/OpenSSL selection:

```text
TLS12_SYSTEM_AUTO=schannel
TLS13_SYSTEM_AUTO=openssl
EXACT_OPENSSL_TLS13=PASS
EXACT_SCHANNEL_TLS13=UNSUPPORTED
ORDERED_OPENSSL_FIRST_TLS12=openssl
CONNECTION_LEVEL_PINNING=PASS
POST_BIND_PROVIDER_SWITCH_COUNT=0
```

Capability filtering happens before binding. Failure after binding remains terminal for that connection; PST never tries another provider as recovery.

### Shutdown and truncation

All providers pass reciprocal clean shutdown, raw-EOF truncation, data-before-truncation preservation and terminal no-resurrection.

M9 exposed and corrected two Schannel production defects without API/SPI change:

1. shutdown now completes after draining a reciprocal `close_notify` that had already been observed;
2. already-buffered TLS is processed before a new socket read is requested.

Schannel and Combined regressions pass after the fixes. Raw EOF without authenticated reciprocal close remains `TRUNCATED`; already-delivered application data remains valid.

### Diagnostics and secret safety

The real-process closure matrix records:

```text
DIAGNOSTIC_CROSS_PROVIDER_MATRIX=PASS
NATIVE_ERROR_REQUIRED_FOR_PUBLIC_CONTROL=NO
LOG_SECRET_HITS=0
PRIVATE_KEY_LOG_HITS=0
APPLICATION_PAYLOAD_LOG_HITS=0
```

Consumers use normalized PST results/diagnostics for control; provider-native error values are not required.

### Stress and lifecycle

M9 ran 250 mixed cycles alternating VC6/NSS, OpenSSL, Schannel and Combined security/lifecycle matrices:

```text
MIXED_STRESS_CYCLES=250
CRASHES=0
HANGS=0
DOUBLE_CLOSE=0
LEAKED_ACCEPTED_TRANSPORT=0
```

Membership/lifetime negatives, failure injection, runtime isolation and active old/new identity snapshots also pass. Established connections keep immutable identity/trust snapshots while new connections can use rotated configuration.

### Current role-scoped masks

M8 locked and M9 reconfirmed:

| Provider | Aggregate | CLIENT | SERVER |
|---|---:|---:|---:|
| OpenSSL | `0x00027fff` | `0x00027eb7` | `0x0000777b` |
| Schannel | `0x00027efd` | `0x00027eb5` | `0x00007679` |
| RetroZilla NSS | `0x00007aff` | `0x00007ab7` | `0x0000727b` |

Three-provider same-process composition remains `NOT_APPLICABLE_CURRENT_TARGET_MATRIX`: the RetroZilla NSS target is x86/VC6 while the current Schannel/OpenSSL Combined target is x64/MSVC. Cross-process network interoperability is the factual evidence across that architecture boundary.

---

## Historical interoperability baseline

The original Phase 7.F work established the first bounded interoperability evidence for the VC6/RetroZilla NSS provider before the later Schannel/OpenSSL and CLIENT/SERVER evolutions. That historical evidence remains valid and is retained in the release-evidence tree; it is not the current limit of PST interoperability.

It established, among other things:

- VC6 public-header consumption;
- real host and Windows NT 4.0 SP6 TLS 1.2/TLS 1.3 with RetroZilla NSS;
- server authentication and mTLS with in-memory DER/PKCS#8;
- custom trust and hostname verification;
- ALPN policy;
- provider-authoritative nonblocking readiness through `PR_Poll`;
- clean close, data then close, and FIN-without-close-notify truncation;
- intermediate root/intermediate/leaf validation with a missing-intermediate negative control;
- negotiated cipher reporting;
- an independent Schannel-backed `.NET SslStream` TLS 1.2 server proof.

The later API 2.0 / SPI 3.0 server-side track extended that baseline to real OpenSSL, Schannel and RetroZilla NSS SERVER implementations. Its SS-6 matrix already proved all nine TLS 1.2 CLIENT×SERVER network pairings and all then-eligible TLS 1.3 pairings. M9 repeats the relevant matrix against the API 2.1 scheduler candidate and adds wait-set/wake/backpressure/upgrade/stress interaction.

## Platform classification

| Platform | Classification | Current evidence/limitation |
|---|---|---|
| Modern Windows x64 development host | TESTED | OpenSSL, Schannel, Combined deterministic and real TLS M0–M9 evidence |
| Win32 x86 VC6 host target | TESTED | RetroZilla NSS/NSPR deterministic and real TLS M0–M9 host evidence |
| Windows NT 4.0 SP6 x86 | TESTED for published 0.5.0 and earlier NSS evidence | M1–M3 mechanisms were statically audited for NT4 compatibility; final 0.6.0 physical NT4 rerun belongs to M10 |
| Windows 2000 | NOT TESTED | not inferred from NT4 |
| Windows XP | NOT TESTED | not inferred from NT4 |
| Windows 95/98 | NOT CURRENTLY VALIDATED | no current release claim |
| Win32s / Windows 3.11 | FUTURE | no current claim |

## Scope boundaries

PST interoperability evidence is transport/TLS evidence, not application-protocol evidence. HTTP/SMTP/IMAP semantics, HTTP pooling, browser navigation, mail account state machines and authorization remain consumer responsibilities.

Likewise, successful execution on one Windows generation does not create a formal OS support range. Target identity, toolchain, architecture, provider capability and `Tested on` evidence remain separate facts.

## Next gate

M0–M9 are complete on `feature/multiplexed-readiness`. M10 must perform final real NT4 validation for the changed 0.6.0 candidate, separate clean-machine package-only validation, deterministic package reproduction, documentation/package audit and post-publication asset/hash verification before 0.6.0 can be claimed as released.