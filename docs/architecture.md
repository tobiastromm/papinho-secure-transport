# Scope and architectural boundaries

Phase 4 materializes immutable credentials/trust, frozen identity configuration, and an owned peer snapshot without changing this boundary. No NSS, NSPR, WinSock, certificate-parser, or crypto type enters the public API.

Phase 5 adds the public runtime and lifecycle core. The generic transport remains opaque; the Win32 adapter is a separate header and implementation. Backend selection is finalized before connection state exists, so no authentication failure can trigger provider fallback.

## Mission

PapinhoSecureTransport (PST) is an independent **portable secure transport abstraction**. Its initial secure protocol is TLS, provided by mature cryptographic/TLS backends. PST is intended for PapinhoAccelerator, PapinhoBrowser, and future consumers without placing any consumer-specific concept in its core.

PST is not a TLS implementation, an NSS or BearSSL wrapper, a generic socket library, a PACC implementation, or a PapinhoAccelerator security-policy engine. The proven RetroZilla NSS/NSPR backend is concrete evidence that informs the architecture; it does not define the entire architecture.

## Responsibility model

```text
Application / consumer protocol
            |
            v
PapinhoSecureTransport
            |
            v
Appropriate underlying transport
            |
            v
Platform / network
```

PST begins where a consumer requests an authenticated, policy-constrained secure channel over an appropriate underlying transport. It ends at secure-transport lifecycle and data transfer. Application framing, business protocol, connection orchestration, and network policy remain with the consumer.

### In scope

- Portable normalization of secure-backend lifecycle and initialization.
- Establishing and operating secure connections over an appropriate underlying transport.
- Incremental, nonblocking handshake and secure read/write behavior.
- Backend-specific secure readiness and progress requirements.
- Peer authentication and certificate/identity information exposed in a backend-neutral form.
- Application of consumer-supplied trust, credential, hostname, ALPN, protocol-version, resumption, early-data, and related secure-transport policy.
- Secure shutdown, close classification, error normalization, and explicit lifetime rules.
- Fail-closed behavior when required policy, authentication, or secure entropy cannot be satisfied.

These are conceptual responsibilities, not API commitments.

### Out of scope

- Custom TLS state machines, AES, ChaCha20, Poly1305, X25519, P-256, HKDF, cryptographic PRNG/DRBG, certificate validation, or other custom cryptography.
- Generic socket abstraction, full transport abstraction, platform abstraction layer, event loop, or build system.
- PACC, CONTROL, DATA, tickets, Accelerator Sessions/capabilities, PCI/ISA, listener policy, egress policy, or TLS offload.
- PapinhoBrowser-specific HTTPS policy or PapinhoAccelerator-specific policy in the generic core.
- Plaintext operation, automatic plaintext fallback, or pseudo-backends such as `PST_BACKEND_PLAINTEXT`, `PST_BACKEND_NONE`, or `PST_TLS_OFF`.
- Public API, ABI, concrete SPI, headers, structs, enums, typedefs, vtables, callbacks, or definitive opaque-handle design during Phase 0.B.

## Consumer boundary

The consumer supplies only information needed to configure and operate secure transport. PST applies that configuration and reports secure-transport outcomes; it does not interpret the consumer's application protocol or business state.

ALPN is consumer configuration and `papacc/1` must not be hardcoded in the generic core. A future PapinhoAccelerator profile may require TLS 1.3, mTLS, ALPN `papacc/1`, disabled 0-RTT, and initially disabled resumption. A future PapinhoBrowser HTTPS profile may instead require server authentication, hostname validation, Web PKI trust, and normally no mandatory mTLS. Neither profile is universal PST policy.

For PapinhoAccelerator, a Secure Principal follows TCP -> TLS 1.3 mTLS -> cryptographic identity -> PACC -> CONTROL/DATA and uses PST. A Legacy Endpoint follows TCP -> plaintext PACC and bypasses PST entirely. Plaintext is never a PST backend or automatic fallback.

## Underlying-transport boundary

TCP is the predominant initial transport, but PST is not synonymous with Winsock or a native TCP socket. PST operates over an appropriate underlying transport without assuming that every platform exposes the same handle, ownership transfer, or polling mechanism. Phase 0.B does not introduce a general-purpose transport abstraction.

No concrete platform handle may leak through the future public interface, including `SOCKET`, `HANDLE`, `PRFileDesc *`, `SSL *`, BearSSL structures, or any other backend-private type.

## Backend boundary

The portable PST core delegates cryptographic and secure-protocol mechanism to backend implementations. The proven legacy NSS/NSPR implementation is the first backend candidate. Modern Windows, Linux/POSIX, embedded, or other backends remain possible, but Phase 0.B neither creates them nor chooses their libraries.

Backend state and platform resources remain private behind a backend-neutral boundary. Phase 2 materializes the internal contract described in [backend-spi.md](backend-spi.md), and Phase 3 implements the first opt-in provider, `retrozilla-nss`, as documented in [backend-nss.md](backend-nss.md). Runtime selection policy remains deferred; neither the SPI nor backend/platform types are exposed publicly.

## Readiness invariant

Transport readiness and secure/backend readiness are distinct. Native `select()` observes the underlying socket, while the validated NSS TLS state is correctly polled with `PR_Poll()` on the SSL-layer descriptor. Therefore PST must not assume:

```text
TLS wants read  == native socket is readable
TLS wants write == native socket is writable
```

The internal SPI represents interest and bounded wait separately without exposing backend handles. The Phase 3 NSS implementation maps those operations to `PR_Poll()` on its private SSL descriptor. Public event-loop integration remains deferred.

## Ownership and lifetime invariants

Ownership must be explicit at every boundary. Ownership of a PST abstraction is distinct from ownership of a concrete transport or backend handle.

The proven NSS layering is native `SOCKET` -> NSPR -> SSL; after import, `PR_Close(ssl_fd)` closes the aggregate. This demonstrates that a backend may assume ownership of an underlying resource. The future design must define whether resources are borrowed, transferred, or retained and must ensure exactly one valid close path. Ambiguous ownership, double-close, use-after-close, and concurrent close by consumer and backend are prohibited.

SPI 2.0 reports irreversible ownership acceptance separately from final attach success. The NSS backend sets acceptance immediately after successful native import and owns every cleanup path thereafter. Public transport construction, retained references, and concurrency remain deferred.

## Mechanism and policy

The backend/PST mechanism performs TLS, supported cryptographic authentication, peer validation, trust processing, ALPN negotiation, handshake, secure I/O, and TLS shutdown.

The consumer/profile defines policy: permitted protocol versions, whether mTLS is required, required ALPN values, resumption and 0-RTT settings, trust anchors, expected hostname, and product-specific security requirements. PST must apply supplied policy correctly and fail closed when it cannot be satisfied. It must not silently downgrade an explicit requirement.

The representation, defaults, validation timing, and composition of policy are deferred to Phase 0.C.

## Entropy and fail-closed security

Failure to obtain cryptographically secure entropy must not fall back to an inadequate source presented as secure. PST does not implement an RNG. The selected legacy backend's normal fail-closed patch and controlled A/B revalidation establish this property for that preserved backend.

`PAPACC_TEST_FORCE_SECURE_RNG_FAILURE` is not a PST feature. It exists only in an external TEST-ONLY / DO NOT SHIP evidence build and must never enter the production runtime or public design.

## Portability boundary

The conceptual core must remain portable across legacy and modern Windows, Linux/POSIX, Raspberry Pi, embedded systems, appliances, and dedicated hardware. C89 and Visual C++ 6.0 (`_MSC_VER=1200`) compatibility are important historical constraints. Hardware attachment such as PCI or ISA belongs to consumers, platform integration, or backends—not the PST core.

Phase 0.B records portability as a constraint. It does not implement a PAL, socket wrapper, compiler compatibility layer, or build system.

## Deliberately deferred to Phase 0.C

- Public API and ABI shape.
- Names and definitions of handles, functions, errors, states, enums, and structs.
- Backend SPI, dispatch/vtable model, registration, selection, and capabilities (descriptor, validation, registry, dispatch, and capabilities were materialized internally in Phase 2; runtime selection remains deferred).
- Underlying-transport attachment and ownership-transfer representation.
- Readiness/progress interface and event-loop integration contract.
- Incremental handshake and I/O call semantics.
- Policy/configuration representation and defaults.
- Credential, trust, peer-identity, certificate, hostname, ALPN, error, and shutdown representations.
- Threading/concurrency contract and exact lifetime state model.

No item in this document should be read as pre-approving a concrete API design.

Phase 0.C resolves these design questions provisionally in [runtime-api-design.md](runtime-api-design.md). That specification remains documentation, not a released public header or implementation.
