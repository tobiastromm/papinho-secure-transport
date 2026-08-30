# Architecture decisions

PapinhoSecureTransport belongs only to secure transport paths. It is a portable abstraction over mature TLS/cryptographic backends and must not implement custom cryptography, random generation, TLS state machines, or certificate validation.

Future design must normalize backend lifecycle, secure connections, incremental handshake, nonblocking read/write, abstract WANT_READ/WANT_WRITE-style progress, backend readiness, close, errors, peer identity, trust/credentials, and backend/entropy initialization. This is a requirements record, not an API.

- Backend handles are opaque; no NSS PRFileDesc or similar backend type is public.
- Readiness belongs to the backend. Native select cannot be assumed; validated NSS readiness uses PR_Poll.
- Socket/backend ownership is explicit. PR_Close on the layered ssl_fd closes the aggregate.
- Nonblocking operation and incremental handshakes are first-class.
- Bounded deadlines belong to consumer/runtime orchestration.
- Consumer security policy is a profile, not generic-core hardcoding. TLS 1.3, mTLS, ALPN papacc/1, 0-RTT off, and initially disabled resumption are a future PapinhoAccelerator profile.

PST does not know about PACC sessions, CONTROL, DATA tickets, PapinhoAccelerator capabilities, egress policy, TLS offload, or Legacy Endpoints. A plaintext Legacy Endpoint bypasses PST. No plaintext/none/TLS-off backend will be created.
