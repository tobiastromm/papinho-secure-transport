# Schannel backend

Status: Phase 8.E TLS 1.2 handshake, readiness, secure I/O, close classification, and shutdown complete. Phase 8.F trust/identity/ALPN/mTLS/peer-info parity is not started.

## Boundary and lifecycle

All SSPI and Schannel types and calls remain in `src/backends/schannel`. The public API, common core, and shared Win32 socket adapter contain no Schannel types. The VC6 target excludes this backend; the modern x64 target excludes RetroZilla NSS.

The backend uses the SDK's `SCH_CREDENTIALS` plus one `TLS_PARAMETERS` record rather than legacy `SCHANNEL_CRED`. A credential handle is acquired once per configured connection because the enabled protocol range belongs to the frozen connection configuration. It is distinct from the per-connection context handle, guarded by its own validity flag, and freed exactly once. There is no global credential handle.

The current server-auth path requires a system trust source, peer authentication, and a non-empty expected hostname. Schannel automatic credential validation receives that hostname in `InitializeSecurityContext`. Custom anchors, native-store client identities, mTLS, ALPN, certificate snapshots, and public peer info remain Phase 8.F work. No certificate-ignore or trust-all flag exists.

## Incremental state and buffers

The handshake tracks first ISC call, credential/context validity, encrypted input, incomplete-message state, provider-allocated output tokens, pending output offsets, handshake completion, negotiated protocol, stream sizes, terminal failure, close observation, and shutdown progress. A step performs bounded work and returns PST `NEED_READ`, `NEED_WRITE`, completion, or failure. Provider wait uses bounded WinSock `select` on the owned nonblocking socket.

Encrypted input is retained across partial records. Both handshake and decrypt preserve `SECBUFFER_EXTRA`. Plaintext larger than a caller buffer is copied into a provider-private remainder and delivered on later reads. ISC tokens allocated by `ISC_REQ_ALLOCATE_MEMORY` are owned explicitly and released with `FreeContextBuffer` after full send or teardown.

`EncryptMessage` uses the queried stream header, maximum-message, and trailer sizes. Application bytes are reported consumed only after the complete encrypted record has been submitted to WinSock. A partial send retains ciphertext and returns `NEED_WRITE`; retries drain it without encrypting the application data twice. `DecryptMessage` never exposes encrypted bytes.

## Protocol, close, and shutdown

TLS policy disables SSL 2/3 and TLS 1.0/1.1 and additionally disables TLS 1.2 or TLS 1.3 when excluded by the requested range. The negotiated protocol is queried through `SECPKG_ATTR_CONNECTION_INFO` and normalized internally; stream sizes come from `SECPKG_ATTR_STREAM_SIZES`.

TLS 1.2 passed against the independent Python/OpenSSL fixture. A real TLS 1.3 attempt failed on the current Windows Schannel runtime, so TLS 1.3 is not advertised. Credential acquisition alone was proven insufficient as an availability probe.

`SEC_I_CONTEXT_EXPIRED` from `DecryptMessage` is authenticated clean close. Raw TCP EOF without that status is `FAILED/TRUNCATED`. Local shutdown applies `SCHANNEL_SHUTDOWN`, obtains the close token through ISC, retains partial output, returns `NEED_WRITE`, and completes after the token drains; it does not wait indefinitely for a reciprocal close.

## Tests

Canonical build and deterministic tests:

```bat
tools\build-modern-msvc.bat clean
tools\build-modern-msvc.bat test
```

Real TLS 1.2, ten 25-byte echoes, and local shutdown:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\run_schannel_runtime_integration.ps1 -TlsVersion 12 -Port 8476 -Exchanges 10
```

The same runner accepts `-CloseMode peer-clean` and `-CloseMode peer-abrupt`. It installs the existing test-only root only in `CurrentUser/Root`, remembers whether it added it, and removes that exact thumbprint in `finally`. Private keys and native errors are not logged.

The deterministic provider test covers partial encrypted input, partial encrypted output with `WSAEWOULDBLOCK`, `SECBUFFER_EXTRA`, plaintext remainder, and fatal wait. The real fixture proves `NEED_WRITE -> NEED_READ` handshake progress, ten secure exchanges, clean peer close, truncated peer close, and incremental close-token output.