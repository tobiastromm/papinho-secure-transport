# M10 external package validation

This internal evidence record summarizes owner-executed validation of the
canonical PapinhoSecureTransport 0.6.0 validation bundles. It is excluded from
the public source package by the canonical staging policy.

## Clean Windows x64

- Windows 10 Pro 19045 x64; MSVC 19.44.35228.
- All five package hashes, layouts, extraction, internal hashes, licensing,
  source/provenance and archive checks passed.
- Six modern CLIENT/SERVER package consumers passed. The VC6 NSS consumer was
  correctly classified as not applicable because that toolchain was absent.
- API 2.1 package-only wait-set, wake, external-source and finite-wait gates
  passed.
- Schannel TLS 1.2 CLIENT/SERVER and truncation passed.
- OpenSSL TLS 1.2/TLS 1.3 CLIENT/SERVER, mTLS, ALPN, 25-byte bidirectional I/O
  and reciprocal shutdown passed.
- Combined EXACT, ORDERED and AUTOMATIC selection, ALPN pre-binding filtering,
  no post-binding fallback, API 2.1 wait-set and real TLS passed.
- Package-local OpenSSL DLL hashes matched the extracted SDK. Trust-store
  counts were unchanged and log-secret hits were zero.

## Windows NT 4.0 SP6 x86

- API 2.1 scheduler/wait-set validation passed.
- NSS CLIENT TLS 1.2 and TLS 1.3 passed.
- NSS SERVER TLS 1.2 passed.
- NSS raw-EOF truncation passed.
- Corrected NSS SERVER TLS 1.3 required-mTLS validation passed with
  `WRITE=25`, `READ=25`, `CONTENT_MATCH=1` and reciprocal shutdown.
- Backend traces prove nonblocking socket setup, `PR_ImportTCPSocket`,
  `PR_SockOpt_Nonblocking`, `SSL_ImportFD`, incremental handshake,
  `PR_WOULD_BLOCK_ERROR`, `PR_Poll`, encrypted I/O, reciprocal close_notify
  and strict truncation.

## TLS 1.3 required-mTLS validation-kit correction

The initial modern test client incorrectly required `fixture/1` ALPN from the
RetroZilla NSS SERVER. The frozen NSS SERVER capability mask does not advertise
complete PST SERVER ALPN semantics. The client therefore correctly negotiated
no ALPN, then aborted before sending the 25-byte payload; the NT4 server
subsequently failed at its read assertion.

The corrected validation runner explicitly configures disabled ALPN and the
modern client sends and expects no ALPN. The real rerun reported:

```text
CLIENT TLS=13 WRITE=25 READ=25 CONTENT_MATCH=1 CLOSE=RECIPROCAL ALPN=NONE PASS=1
NSS_SERVER_DIRECT TLS=0x0304 READ=25 WRITE=25 CONTENT_MATCH=1 ALPN=- SHUTDOWN=RECIPROCAL PASS=1
PAPINHOSECURETRANSPORT M10 NT4 PACKAGE TLS 1.3 MTLS PASS
```

Classification and scope:

```text
NT4_TLS13_MTLS_FAILURE_CLASSIFICATION=VALIDATION_KIT_DEFECT
NSS_SERVER_ALPN_ADVERTISED=NO
TLS13_MTLS_WITHOUT_UNSUPPORTED_ALPN_EXPECTATION=PASS
REAL_NT4_TLS13_MTLS=PASS
PRODUCTION_CODE_CHANGED=NO
API_CHANGED=NO
SPI_CHANGED=NO
THIRD_PARTY_CHANGED=NO
```

All earlier accepted NT4 results remain valid because the correction affected
only this gate's unsupported ALPN expectation.
