<!-- SPDX-License-Identifier: MPL-2.0 -->

# Providers and selection

SPI 3.0 makes CLIENT and SERVER independent factual capabilities and selects an eligible provider per connection. All three current providers implement both roles, with deliberately asymmetric role masks.

A provider is the engine adapter that implements PST's common secure-transport contract.

| Capability | RetroZilla NSS | Schannel | OpenSSL 3.5.8 |
|---|---:|---:|---:|
| TLS 1.2 | yes | yes | yes |
| TLS 1.3 CLIENT / SERVER | yes / yes | not advertised / not advertised on tested Win10 19045 | yes / yes |
| CUSTOM_TRUST | yes | yes | yes |
| SYSTEM_TRUST CLIENT / SERVER | no / no | yes / yes | yes / yes, through Windows chain evaluation |
| ALPN CLIENT / SERVER | yes / no | yes / no | yes / yes |
| PEER_NAME_VERIFY CLIENT / SERVER | yes / no | yes / no | yes / no |
| full independent SNI control CLIENT / SERVER | no / no | yes / no | yes / no |
| LOCAL_IDENTITY / peer certificate auth / PEER_INFO | yes | yes | yes |
| NONBLOCKING / BACKEND_WAIT | yes | yes | yes |

Exact API 2.1/SPI 3.0 role masks are:

| Provider | Aggregate | CLIENT | SERVER |
|---|---:|---:|---:|
| RetroZilla NSS | `0x00007aff` | `0x00007ab7` | `0x0000727b` |
| Schannel | `0x00027efd` | `0x00027eb5` | `0x00007679` |
| OpenSSL 3.5.8 | `0x00027fff` | `0x00027eb7` | `0x0000777b` |

The aggregate is exactly the union of the two role masks; it is discovery metadata, not permission to combine bits from different roles. `PEER_CERT_AUTH` covers required authentication, `PEER_CERT_OPTIONAL` is additionally required for OPTIONAL, and DISABLED is the baseline policy and needs no capability bit. mTLS is a configuration using the proven local-identity and peer-authentication capabilities, not a separate public bit.

Wait-set scheduling, mixed borrowed external sources, wake, bounded partial I/O, TLS attach after a clean plaintext boundary, reciprocal shutdown and strict truncation are proven provider behaviors rather than additional public capability bits. All providers preserve provider-authoritative TLS readiness; RetroZilla NSS specifically retains `PR_Poll` authority after native wake-up. OpenSSL has full per-runtime provider isolation, Schannel has partial isolation because Windows certificate-store adaptation is process-external state with owned cleanup, and the RetroZilla NSS snapshot retains its documented process-global singleton limitation.

NSS targets legacy Win32/NT4 and has a provider-local singleton. Schannel is native Windows; its SERVER intermediate-chain delivery may install only required intermediates in CurrentUser\\CA with shared ownership accounting and normal cleanup. A crash can leave a PST-inserted intermediate because safe stale-entry deletion cannot distinguish every ownership history. OpenSSL is the staged 3.5.8 target with custom trust and Windows SYSTEM_TRUST integration.

OpenSSL and Schannel CLIENT advertise complete independent `SNI_CONTROL`. RetroZilla NSS preserves its narrower historical `SSL_SetURL` COMPAT behavior but does not advertise that bit; EXPLICIT or DISABLED-with-independent-name configurations therefore filter NSS before binding. None of the current SERVER roles advertises CLIENT SNI control.

After public bootstrap, EXACT tries only the named provider. ORDERED considers caller IDs in order. AUTOMATIC walks built-in registration order and filters by requested role and required capabilities before binding. It does not fall back after binding because a handshake, authentication, trust, transport, I/O or shutdown operation failed. In the combined target, Schannel is registered first: eligible TLS1.2 SERVER without ALPN may select Schannel, while SERVER ALPN or TLS1.3 skips it before binding and selects OpenSSL.

`pst_win32_register_retrozilla_nss()` remains a supported specific compatibility helper; general programs should use `pst_win32_register_builtin_providers()`.
