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
| LOCAL_IDENTITY / peer certificate auth / PEER_INFO | yes | yes | yes |
| NONBLOCKING / BACKEND_WAIT | yes | yes | yes |

NSS targets legacy Win32/NT4 and has a provider-local singleton. Schannel is native Windows; its SERVER intermediate-chain delivery may install only required intermediates in CurrentUser\\CA with shared ownership accounting and normal cleanup. A crash can leave a PST-inserted intermediate because safe stale-entry deletion cannot distinguish every ownership history. OpenSSL is the staged 3.5.8 target with custom trust and Windows SYSTEM_TRUST integration.

After public bootstrap, EXACT tries only the named provider. ORDERED considers caller IDs in order. AUTOMATIC walks built-in registration order and filters by requested role and required capabilities before binding. It does not fall back after binding because a handshake, authentication, trust, transport, I/O or shutdown operation failed. In the combined target, Schannel is registered first: eligible TLS1.2 SERVER without ALPN may select Schannel, while SERVER ALPN or TLS1.3 skips it before binding and selects OpenSSL.

`pst_win32_register_retrozilla_nss()` remains a supported specific compatibility helper; general programs should use `pst_win32_register_builtin_providers()`.
