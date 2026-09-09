<!-- SPDX-License-Identifier: MPL-2.0 -->

# Migration from API 1.3 / SPI 2.4 to API 2.0 / SPI 3.0

The published v0.4.0 contract remains historical and unchanged. The 0.5.0 development line intentionally breaks source and ABI compatibility because no deployed external consumer requires preservation of ambiguous concepts.

| API 1.3 / SPI 2.4 | API 2.0 / SPI 3.0 |
|---|---|
| runtime selects one provider | runtime is provider context; connection selects provider |
| `PST_RUNTIME_OPTIONS` carries selection | `PST_CONNECTION_CONFIG.provider_selection` |
| `PST_RUNTIME_INFO.backend_id` | runtime provider enumeration; connection selected-provider info |
| mutable opaque `pst_config` + freeze | complete transactional `PST_CONNECTION_CONFIG` at create |
| `PST_IDENTITY_CONFIG.credentials` | `PST_LOCAL_IDENTITY.credentials` |
| `trust` mixed into identity | `PST_PEER_AUTH_CONFIG.trust` |
| `expected_hostname` | `expected_peer_name` |
| `require_peer_authentication` | DISABLED/OPTIONAL/REQUIRED peer-certificate mode |
| `require_client_authentication` | SERVER peer-certificate policy; removed name |
| `PST_CAP_CLIENT_AUTH` | LOCAL_IDENTITY and PEER_CERT_AUTH capabilities |
| one `PST_CAP_ALPN` | ALPN_CLIENT and ALPN_SERVER |
| `PST_CAP_HOSTNAME_VERIFY` | PEER_NAME_VERIFY |
| ALPN embedded in TLS policy | distinct role-dependent `PST_ALPN_CONFIG` |
| `hostname_validated` fact | `peer_name_validated`, including NOT_APPLICABLE |
| SPI configure-identity hook | transactional role-aware SPI connection create |

Existing target IDs do not change: platform, architecture, toolchain and provider identity are unchanged. Versions become API 2.0.0, SPI 3.0 and library/package 0.5.0 development; nothing is published by this migration.

## Consumer migration checklist

1. Register the built-in providers before creating a runtime, but move provider selection from runtime options into each `PST_CONNECTION_CONFIG`.
2. Set an explicit CLIENT or SERVER role. Construct Local Identity separately from Peer Authentication and its exclusive Peer Trust object. CLIENT Expected Peer Name is independent of trust; omit it for SERVER.
3. Split TLS and ALPN configuration. Treat CLIENT ALPN as an offer and SERVER ALPN as local preference, and request only capabilities present in that provider's role mask.
4. Pass PST only an already-connected transport. SERVER consumers retain listener ownership. Honor `ownership_accepted`: before acceptance the caller closes; after acceptance PST/provider owns the single close root.
5. Drive handshake, I/O, wait and reciprocal shutdown incrementally. Do not interpret local close_notify emission as completed shutdown or established raw EOF as clean closure.
6. Query the selected provider from the connection, and use copied Peer Info/diagnostics. Authentication facts are not application authorization.

## Provider-author SPI migration checklist

Replace the SPI 2.4 construction/configuration split with one transactional `connection_create` receiving immutable `PST_BACKEND_CONNECTION_OPTIONS`. Publish aggregate, CLIENT and SERVER masks; aggregate is only their discovery union. Validate the requested role and configuration before publishing state. Implement only factual role-scoped capabilities, preserve provider-neutral transport/ownership semantics, copy diagnostics/Peer Info, and keep all native state private.

Initialization remains lazy. Selection may continue past an ineligible or initialization-failed candidate only before binding. Once transport ownership is accepted, no backend result can request fallback. Provider cleanup must cover partial creation, attach failure, terminal handshake/auth/trust failures, shutdown and destroy with exactly one transport close.
