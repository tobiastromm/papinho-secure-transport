# RetroZilla NSS/NSPR backend

Phase 4 adds backend-neutral custom trust, memory DER/PKCS#8 local credentials, client authentication, and independent peer snapshots. Phase 3 environment overrides remain test-only compatibility mechanisms, not the Phase 4 contract. See [credentials-trust-peer.md](credentials-trust-peer.md).

Status: Phase 3 backend implementation. The backend is private and opt-in; it is not part of the default core build and exposes no NSS/NSPR type through the public header.

## Lineage and preserved runtime

The backend ID is `retrozilla-nss`. It targets the preserved RetroZilla NSS 3.42 Beta / NSPR 4.7.7 Win32 x86 build produced with Visual C++ 6.0. The normal runtime remains under `third_party/retrozilla-nss/prebuilt/win32-x86-vc6/runtime`.

The authorized fail-closed pair was rechecked during Phase 3:

- `freebl3.dll`: SHA-256 `12808c651528c9e08f5ccf86af00f9061b19103c0c712d376755664f41ee474d`;
- `freebl3.chk`: SHA-256 `122aefebbfd76eb68352eb23d59d4caff70183b520f81b5017bf299f7b745daa`.

The failure-injection-v3 artifacts remain outside the normal runtime and manifests. The PST backend adds no entropy source or fallback. Initialization errors from NSS are normalized as backend failure and terminate initialization without returning usable state.

## Build and dependency boundary

The default `nmake /f Makefile.vc6 test` build does not compile or link the NSS backend. Backend-specific targets require an explicit `NSS_DIST` pointing at the matching legacy SDK headers:

```bat
nmake /f Makefile.vc6 NSS_DIST=C:\path\to\dist test-nss-unit
nmake /f Makefile.vc6 NSS_DIST=C:\path\to\dist nss-integration
```

The backend dynamically resolves the required entry points from `nspr4.dll`, `nss3.dll`, and `ssl3.dll`. The operator must place the preserved runtime directory in the process DLL search path. This keeps NSS import libraries and headers out of consumers and out of the portable core. A future runtime/configuration layer must provide an explicit deployment-safe runtime-location policy; Phase 3 does not introduce DLL discovery.

## Lifecycle and state

`initialize` loads the legacy modules, resolves every required symbol, initializes NSPR, and invokes `NSS_NoDB_Init(NULL)` by default. If the private test environment variable `PST_NSS_DB_DIR` is present, it invokes `NSS_Init` with that directory so an opt-in integration fixture can use an external NSS certificate database.

Only one active NSS backend state is permitted in this initial implementation because NSS/NSPR lifecycle is process-global. A repeated initialize while active returns `PST_RESULT_INVALID_STATE`. Registry setup and backend lifecycle remain serialized by the caller; Phase 5 owns production synchronization.

Backend, runtime, and connection states are separately allocated and opaque to the core. Destruction closes an attached SSL aggregate once, destroys connection/runtime state, calls `NSS_Shutdown`, calls `PR_Cleanup`, unloads modules, and frees backend state. Destruction performs no unbounded handshake loop.

Phase 3 executed this lifecycle using the normal versioned DLLs on Windows 10 Pro 10.0.19045 x64 with the VC6-built Win32 x86 test executable. This is not formal NT4 validation; that remains Phase 6.

## Descriptor and capabilities

The descriptor advertises only paths implemented in this backend:

- TLS 1.2 and TLS 1.3 mechanisms provided by the preserved NSS build;
- hostname verification setup through `SSL_SetURL` and NSS certificate authentication when a DB fixture is configured (the runtime query omits this bit in NoDB mode);
- incremental nonblocking operation;
- backend-correct waiting.

It does not advertise client authentication, ALPN, custom/system trust objects, resumption, early data, or peer-info extraction. Historical harness evidence for those features is not treated as a PST implementation claim.

## Native transport and ownership

`PST_NSS_NATIVE_TRANSPORT` is private to `src/backends/nss`. It carries a Win32 socket value and certificate hostname without putting `SOCKET` in the public API or generic SPI.

Only TRANSFERRED ownership is currently accepted. The attach path is:

1. set the native socket nonblocking with `FIONBIO`;
2. import it through `PR_ImportTCPSocket`;
3. mark `ownership_accepted = 1` at that irreversible successful import;
4. set `PR_SockOpt_Nonblocking`;
5. layer `SSL_ImportFD`;
6. enable SSL security/client handshake, disable SSL 3.0, set the hostname, reset the handshake, and install NSS certificate authentication when a DB is configured.

Before step 3, failure leaves ownership with the caller. At or after step 3, the backend owns the imported resource and closes the current NSPR/SSL aggregate on every failure path. The caller must invalidate its former handle whenever `ownership_accepted` is one, even if attach ultimately reports failure. This prevents both double-close and leaks.

BORROWED and RETAINED attachment are rejected until a later transport adapter can implement their lifetime contracts safely.

## Nonblocking operations and readiness

`handshake_step` calls `SSL_ForceHandshake` once. `PR_WOULD_BLOCK_ERROR` becomes NEED_READ_WRITE, not a fatal result. No internal retry loop or hidden wait exists.

The conservative READ|WRITE interest is intentional: NSS can require traffic opposite to the apparent application operation. `wait` maps the current interest to a private `PRPollDesc` and calls `PR_Poll` on the SSL descriptor with a bounded millisecond timeout. Timeout, ready interest, hangup, and poll failure remain distinct. No NSPR descriptor escapes the backend.

`read` and `write` call `PR_Read`/`PR_Write` once and report bytes independently from operation state and normalized error. Positive partial progress is preserved. Would-block produces NEED_READ_WRITE. A zero read is classified as clean closed; reset/EOF errors are classified as truncated. Fatal results preserve the native code in private connection state.

`shutdown_step` calls `PR_Shutdown(PR_SHUTDOWN_BOTH)` once. Success is COMPLETE, would-block is NEED_READ_WRITE, and other failures are normalized. Connection destruction remains immediate and bounded and is not represented as a completed graceful TLS shutdown.

## Error normalization

The backend recognizes:

- `PR_WOULD_BLOCK_ERROR` as normal progress;
- reset/EOF as truncation;
- selected NSPR connectivity/I/O failures as transport failure;
- hostname mismatch separately;
- identifiable certificate errors and certificate alerts as authentication failure;
- SSL-range errors as protocol failure;
- NSS-range/internal errors as backend failure;
- NSS out-of-memory separately.

The most recent native code remains private and can be read by backend-specific tests. Portable control flow uses only `PST_RESULT` and SPI operation states.

## Tests and current limitation

`test_backend_nss` validates descriptor registration, advertised capabilities, error normalization, duplicate handling, and optionally the real NSS/NSPR lifecycle. Its default mode neither loads DLLs nor uses a network.

`test_backend_nss_integration` is a separately built opt-in loopback client. It accepts host, port, and certificate hostname, imports the connected socket through the PST SPI, and drives handshake through incremental step plus backend `PR_Poll`. For successful certificate authentication it requires an external test NSS DB selected with `PST_NSS_DB_DIR` and a separately managed local TLS server.

The integration executable was executed with an ephemeral private CA/server fixture on loopback. The PST backend completed an authenticated TLS 1.2 handshake after two would-block/poll cycles, echoed 27 secure bytes, and completed its shutdown step. Every NSS/NSPR module path was recorded under the versioned repository runtime. See [phase3-functional-proof.md](codex/phase-history/phase3-functional-proof.md). No fixture or public credential/trust API was retained.

ALPN, client credentials/mTLS, public trust loading, peer snapshots, public runtime selection, and consumer-facing transport setup remain deferred to Phases 4 and 5. No application protocol identifier is compiled into this backend.
