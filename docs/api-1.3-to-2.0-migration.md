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
