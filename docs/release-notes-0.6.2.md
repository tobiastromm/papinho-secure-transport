# PapinhoSecureTransport 0.6.2 release notes

PapinhoSecureTransport 0.6.2 makes the VC6 C runtime model an explicit binary
artifact identity. It preserves public API 2.1.0, provider SPI 3.0, the existing
TLS/provider behavior, and the static-library distribution model.

## Why the CRT variants exist

The historical VC6/RetroZilla NSS package used the unsuffixed target identity
`win32-x86-vc6-retrozilla-nss` and the `/ML` runtime model. A real
PapinhoBrowser integration requires `/MD`; the two static-library variants are
not interchangeable merely because they use the same compiler and provider.

Starting with 0.6.2, new VC6 packages therefore use explicit identities:

```text
win32-x86-vc6-retrozilla-nss-ml  -> compile/link compatible /ML consumers
win32-x86-vc6-retrozilla-nss-md  -> compile/link compatible /MD consumers
```

The historical unsuffixed 0.6.1 artifact remains immutable and is not renamed
or replaced. No `/MT` package is provided or claimed.

## Binary SDK matrix

| Target | Provider(s) | CRT guidance |
|---|---|---|
| `win32-x86-vc6-retrozilla-nss-ml` | RetroZilla NSS/NSPR | VC6 `/ML` consumers only |
| `win32-x86-vc6-retrozilla-nss-md` | RetroZilla NSS/NSPR | VC6 `/MD` consumers, including PapinhoBrowser |
| `win32-x64-msvc-19.51-schannel` | Windows Schannel | documented modern MSVC `/MD` target |
| `win32-x64-msvc-19.51-openssl3` | OpenSSL 3.5.8 LTS | documented modern MSVC `/MD` target |
| `win32-x64-msvc-19.51-schannel-openssl3` | Schannel + OpenSSL | optional Combined provider-selection target |

Each SDK records its exact target, toolchain, provider, CRT, source commit,
corresponding source package and internal SHA-256 manifest. Select the package
whose CRT matches the consumer; do not use `/NODEFAULTLIB` to mix variants.

## Validation

- real Windows NT 4.0 SP6 x86 `/ML`: PASS;
- real Windows NT 4.0 SP6 x86 `/MD`: PASS using the existing system
  `MSVCRT.DLL`, without installing a replacement;
- separate clean-machine `/ML` and `/MD` package-only TLS 1.3: PASS;
- CLIENT TLS 1.2/TLS 1.3, SERVER TLS 1.2/TLS 1.3 required mTLS, encrypted
  25-byte I/O, reciprocal shutdown, truncation and NSS `PR_Poll`: PASS;
- real PapinhoBrowser `/MD` consuming only the PST `/MD` SDK, TLS 1.3: PASS;
- first ML/MD functional or lifecycle divergence: none.

## Provider limitations

Provider capability masks remain factual and role-scoped. Schannel SERVER does
not advertise TLS 1.3 or complete PST SERVER ALPN semantics on the validated
environment. RetroZilla NSS SERVER does not advertise SYSTEM trust or complete
PST SERVER ALPN semantics. RetroZilla NSS independent CLIENT SNI control remains
partial. Unsupported requirements are filtered before binding, with no
post-binding provider fallback.

## Public assets and hashes

The release publishes one corresponding-source ZIP, the five SDK ZIPs above,
and `SHA256SUMS-packages.txt`. The final SHA-256 values are frozen by the M5-A
reproduction gate and must match the release assets byte-for-byte.
