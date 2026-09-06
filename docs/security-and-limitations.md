<!-- SPDX-License-Identifier: MPL-2.0 -->

# Trust, security, readiness, and limitations

PST requires TLS 1.2 or newer and fails closed. It does not silently downgrade, bypass hostname validation, union CUSTOM_TRUST with SYSTEM_TRUST, or fall back to plaintext. mTLS client identity is explicit certificate DER plus unencrypted PKCS#8 DER; server authentication and client authentication are distinct. Required ALPN mismatch fails.

CUSTOM_TRUST is an application-supplied CA set for private, corporate, laboratory, or pinned deployment PKI. SYSTEM_TRUST delegates to a capable provider. OpenSSL's Windows adapter performs per-connection Windows chain evaluation without PST online revocation, AIA fetching, or root auto-update during evaluation. Enterprise/Group Policy stores are structurally inherited but are not domain-runtime certified here. A future PapinhoTrustStore could be a portable, versioned, authenticated CUSTOM_TRUST source; it is not implemented and PST is not a public root program.

Operations are incremental and bounded. READ/WRITE/READ_WRITE readiness means retry may be useful, not that TLS progressed. A received TLS `close_notify` is CLEAN; unexpected EOF/reset is TRUNCATED. Graceful shutdown completes after reciprocal `close_notify`; if PST sends `close_notify` and the peer then closes TCP without replying with its own alert, the provider may report unexpected EOF and PST preserves it as `PST_RESULT_TRUNCATED`. Plaintext received before close remains deliverable. Shutdown is incremental.

Diagnostics are copied normalized result/operation/backend-ID snapshots, not a global last-error and not native error disclosure. Logging is a temporal stream of structured events; diagnostic snapshots are copied normalized state. Logging levels OFF=0 through TRACE=5 are cumulative. Callbacks are synchronous; events are ephemeral and context is caller-owned. PST provides no worker or file logger. The public event and diagnostic records have no fields for hostnames, application payload, certificate DER, keys, trust material, paths, native handles/pointers, or arbitrary native provider error codes. Applications remain responsible for sanitizing any data they add to their own logs.

Built-in providers have different advertised capabilities. Selection is constrained by the requested policy and selection mode; PST does not imply that providers are interchangeable or that every provider supports every TLS version or trust model. PST handles secure transport within its documented contract. The application remains responsible for its protocol semantics, authorization decisions, credential handling outside PST, and application-level data behavior.

## Tested and not claimed

Real evidence covers Windows NT 4.0 SP6 for the preserved NSS package and Windows 10 build 19045 for Schannel, plus modern-host OpenSSL tests. It does not imply Windows 2000, XP, 95, 98, Win32s, every Windows build, or every enterprise domain configuration. Current public support does not claim POSIX adapters, dynamic plugins, session resumption, 0-RTT, a public full-chain API, DTLS, QUIC, Noise, FIPS certification, formal verification, or universal revocation behavior.