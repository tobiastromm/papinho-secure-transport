<!-- SPDX-License-Identifier: MPL-2.0 -->

# Schannel backend

Status: Phase 8 Schannel work is complete. TLS 1.2, system/custom trust, hostname, ALPN, explicit mTLS, peer info, readiness, close, shutdown and same-runtime isolation passed; TLS 1.3 is unavailable and not advertised on the tested runtime.

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
## Phase 8.F partial validation (blocked)

Schannel now requests manual credential validation. After `SEC_E_OK`, the provider obtains the remote `CERT_CONTEXT`, builds a server-auth chain with cache-only/AIA-disabled retrieval, and applies `CERT_CHAIN_POLICY_SSL` with the explicitly configured hostname. System trust uses the native chain engine. Custom trust uses a memory store and an exclusive-root chain engine, so it cannot fall back to an otherwise trusted system root. No production path mutates `CurrentUser\Root`.

The real TLS 1.2 custom-trust chain `root -> intermediate -> leaf` passed with 25-byte secure echo. A wrong custom root returned `AUTH_FAILURE`, wrong hostname returned `HOSTNAME_MISMATCH`, and an omitted intermediate returned `AUTH_FAILURE`. The system-trust negative also returned `AUTH_FAILURE`. The required system-trust positive remains unproved: the local OpenSSL fixture uses a private CA, and installing that CA in `CurrentUser\Root` would violate the phase's no-injection gate.

ALPN is supplied on the first ISC call through `SECBUFFER_APPLICATION_PROTOCOLS`. Real negotiation preserved offered order and selected `fixture/2` from `fixture/1,fixture/2`; required-with-no-selection returned `POLICY_VIOLATION`; optional-with-no-selection stayed established and returned `UNAVAILABLE`. These gates justify advertising `ALPN`.

Peer info uses `SECPKG_ATTR_REMOTE_CERT_CONTEXT`, `SECPKG_ATTR_CIPHER_INFO`, and `CryptHashCertificate2(SHA256)`. The real snapshot reported TLS 1.2, standard cipher suite `0xC030`, validation/authentication flags, 32-byte SHA-256, and an owned 862-byte leaf DER. Summary and DER remained readable after connection destruction. These gates justify advertising `PEER_INFO`.

The explicit PKCS#8 client-identity path now passes. `PKCS12_NO_PERSIST_KEY` produced an ephemeral key context that this Schannel runtime did not accept for `AcquireCredentialsHandle`; importing the in-memory PFX with `CRYPT_USER_KEYSET | PKCS12_ALWAYS_CNG_KSP` creates a uniquely named provider-visible temporary CNG key. The in-memory certificate exposes `CERT_KEY_PROV_INFO_PROP_ID`, stays alive for the credential/context lifetime, and is supplied through `SCH_CREDENTIALS.cCreds = 1` and `paCred`. `AcquireCredentialsHandle` returned `SEC_E_OK`, while `SCH_CRED_NO_DEFAULT_CREDS` remains enabled. Cleanup deletes the temporary key after the credential and certificate lifetimes, and enumeration reported `PST_MTLS_KEY_REMAINS=0`.

A real TLS 1.2 mTLS exchange passed with 25-byte bidirectional I/O, exact client-certificate SHA-256 `MATCH=1`, ALPN `fixture/1`, authenticated peer information, and incremental shutdown. The no-client-credential case remains fail-closed as `AUTH_FAILURE`. These gates justify advertising `CLIENT_AUTH`. The explicit wrong-client identity, same-runtime configuration-isolation, and OFF/ERROR/TRACE client-auth logging gates remain for final 8.F completion.

The PowerShell runner now emits bounded stage markers from `RUNNER_START` through `RUNNER_END`, preserves child output, and captures both process exit codes. Its first failure was an invalid direct conversion of `SwitchParameter` to `Int32`; after using `ClientAuth.IsPresent` and retaining the process handle, the custom-trust mTLS run ended with client/server exit code 0 and `SUCCESS=1`. Phase 8.F remains in progress because the system-trust positive, wrong-client, configuration-isolation, and logging gates are still open; Phase 9 and additional backends have not started.

### Phase 8.F3 final-gate evidence

A structurally valid client certificate and matching PKCS#8 key signed by a separate untrusted root reached `AcquireCredentialsHandle` successfully and was rejected during the real TLS handshake. Python/OpenSSL reported `CERTIFICATE_VERIFY_FAILED: unable to get local issuer certificate`; PST returned `AUTH_FAILURE`, retained a `schannel` diagnostic, and all handshake/read/write resurrection attempts were rejected. The no-credential case still returned `AUTH_FAILURE`, proving default Windows client-certificate selection remains disabled.

The identical wrong-client scenario produced `LOG_EVENTS=0` at OFF, exactly one ERROR and no WARN at ERROR, and a bounded 12 events (eight TRACE progress plus exactly one ERROR) at TRACE. Public events contain only the provider-neutral fixed ABI; certificate/key bytes, identities, hostnames, ALPN strings, key-container names, native status, provider/store names, and handles are absent. A single transient run normalized the peer rejection/close race as `BACKEND_FAILURE`; bounded repetitions at ERROR and TRACE consistently returned `AUTH_FAILURE`, so this is recorded as fixture timing sensitivity rather than logging-dependent behavior.

No deterministic local already-trusted server certificate and private key are available without relying on accidental machine state. The permitted environment-dependent probe therefore connected with SYSTEM trust to `www.microsoft.com:443`, sent no application bytes, completed TLS 1.2 with certificate/chain/hostname/authentication flags true, cipher `0xC030`, a 32-byte SHA-256 fingerprint and owned leaf DER, then completed shutdown. No trust store was modified.

The required B-valid -> C-wrong -> A-system -> B-valid sequence within one `pst_runtime` is not proved by the existing one-connection-per-process fixtures. Separate-process results cannot honestly satisfy that requirement. Phase 8.F remains open solely for a dedicated same-runtime multi-connection integration harness and its final cleanup/diagnostic assertions.


### Phase 8.F4 single-runtime isolation closure

A dedicated modern integration harness executes B1 valid mTLS -> C wrong mTLS -> A SYSTEM/no-client-credential -> B2 valid mTLS in one process and one `pst_runtime`. It creates and destroys four independent frozen configurations/connections, releases the runtime once, and keeps runtime logging OFF while relying on the completed 8.F3 logging matrix.

B1 and B2 each negotiated `fixture/1`, presented the exact expected explicit identity, exchanged the 25-byte payload, reported no diagnostic, and completed shutdown. C used the same custom trust, hostname, and ALPN policy with the structurally valid wrong identity; the server rejected its chain, PST returned terminal `AUTH_FAILURE`, and its copied diagnostic remained immutable after destruction. A used SYSTEM trust, `www.microsoft.com`, no ALPN, no client credential, and zero application bytes; authenticated chain and hostname validation passed without inheriting custom trust or identity state.

The run reported `RUNTIME_CREATE_COUNT=1`, `RUNTIME_RELEASE_COUNT=1`, `CONNECTION_CREATE=4`, `CONNECTION_DESTROY=4`, `IDENTITY_SETUP=3`, `IDENTITY_CLEANUP=3`, `PST_MTLS_KEY_REMAINS=0` after every case, final capability mask `0x00000e7d`, and `ISOLATION_END PASS=1`. No production change was required. With the last configuration-isolation blocker closed, Phase 8.F is complete.
