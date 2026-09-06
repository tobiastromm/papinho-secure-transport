<!-- SPDX-License-Identifier: MPL-2.0 -->

# Providers and selection

A provider is the engine adapter that implements PST's common secure-transport contract.

| Capability | RetroZilla NSS | Schannel | OpenSSL 3.5.8 |
|---|---:|---:|---:|
| TLS 1.2 | yes | yes | yes |
| TLS 1.3 | yes | not advertised on tested Win10 19045 | yes |
| CUSTOM_TRUST | yes | yes | yes |
| SYSTEM_TRUST | no | yes | yes, through Windows chain evaluation |
| HOSTNAME_VERIFY / ALPN / mTLS / PEER_INFO | yes | yes | yes |
| NONBLOCKING / BACKEND_WAIT | yes | yes | yes |

NSS targets legacy Win32/NT4 and has a provider-local singleton. Schannel is native modern Windows. OpenSSL is the staged 3.5.8 target with custom trust and Windows SYSTEM_TRUST integration.

After public bootstrap, EXACT tries only the named provider. ORDERED considers caller IDs in order. AUTOMATIC walks built-in registration order and filters by required capabilities. It does not fall back after runtime selection because a handshake, authentication, or I/O operation failed. In the combined target, Schannel is registered first: TLS1.2+SYSTEM normally selects Schannel, while TLS1.3+SYSTEM skips incompatible Schannel and selects OpenSSL.

`pst_win32_register_retrozilla_nss()` remains a supported specific compatibility helper; general programs should use `pst_win32_register_builtin_providers()`.