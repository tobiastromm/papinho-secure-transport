<!-- SPDX-License-Identifier: MPL-2.0 -->

# Phase 3 NSS functional proof

Date: 2026-08-31. System: Windows 10 Pro 10.0.19045 x64. Client executable: Win32 x86 built with Visual C++ 6.0 `/W4`.

## Isolation audit

WinSock-specific production code is confined to `src/backends/nss/pst_backend_nss.c`. The opt-in integration client uses WinSock in `tests/test_backend_nss_integration.c`. `SOCKET`, `FIONBIO`, `ioctlsocket`, and `closesocket` do not occur in the public header, portable core, or generic SPI. `select` does not occur in the implementation or test. The backend-specific transport record carries an opaque-width native value and is private to `src/backends/nss`.

## Runtime integrity

The process DLL search path was reduced to the repository runtime plus Windows system directories. The external archive and failure-injection-v3 were not used.

- `freebl3.dll`: `12808c651528c9e08f5ccf86af00f9061b19103c0c712d376755664f41ee474d`
- `freebl3.chk`: `122aefebbfd76eb68352eb23d59d4caff70183b520f81b5017bf299f7b745daa`

Loaded module paths recorded by the backend process:

- `nspr4.dll`, `nss3.dll`, `ssl3.dll`, `nssutil3.dll`, `plc4.dll`, `plds4.dll`, `softokn3.dll`, and `freebl3.dll` all loaded from `C:\Projetos\PapinhoSecureTransport\third_party\retrozilla-nss\prebuilt\win32-x86-vc6\runtime`.

## Private fixture

OpenSSL from the existing Git installation generated an ephemeral CA and localhost server certificate under the user temporary directory. A Python standard-library TLS server listened only on `127.0.0.1` on a dynamically assigned port and echoed one payload. The backend imported the CA DER as a temporary NSS certificate and set CA trust using `CERT_NewTempCertificate` and `CERT_ChangeCertTrust`. NSS performed certificate and hostname validation through `SSL_AuthCertificate`; no public credential/trust API or custom certificate validation was added.

The temporary private key, certificates, server script, logs, and CA were removed after evidence capture and were never added to Git.

## Executed PST path

The VC6 integration executable exercised:

```text
PST SPI
-> retrozilla-nss
-> private Win32 socket transport
-> FIONBIO
-> PR_ImportTCPSocket (ownership_accepted=1)
-> PR_SockOpt_Nonblocking
-> SSL_ImportFD
-> SSL_ForceHandshake
-> PR_WOULD_BLOCK_ERROR
-> PR_Poll on the private SSL descriptor
-> handshake completion
-> PR_Write / PR_Read echo
-> PR_Shutdown
```

Trace summary:

```text
FIONBIO ok
PR_ImportTCPSocket ok ownership_accepted=1
PR_SockOpt_Nonblocking ok
SSL_ImportFD ok
SSL_ForceHandshake PR_WOULD_BLOCK_ERROR
PR_Poll call
PR_Poll ready
SSL_ForceHandshake PR_WOULD_BLOCK_ERROR
PR_Poll call
PR_Poll ready
SSL_ForceHandshake complete
PR_Write progress
PR_Read progress
PR_Shutdown complete
```

Result:

```text
TLS_VERSION=0x0303 WRITE=27 READ=27 SHUTDOWN=0
test_backend_nss_integration: PASS
```

This proves TLS 1.2, authenticated localhost server identity, nonblocking incremental progress, backend-correct polling, secure payload echo, and bounded shutdown through the new PST backend. It is not NT4 validation, mTLS validation, or proof of every advertised native NSS capability; those remain separately scoped.

## SPI 2.0 ownership transition

SPI 2.0 added `ownership_accepted` because a backend can successfully cross an irreversible transport-ownership boundary before later secure-layer setup finishes. Before the call, the caller owns the resource. The output starts at zero. Failure while zero leaves close responsibility with the caller. Once a backend successfully consumes a TRANSFERRED resource, it sets the output to one and becomes responsible for cleanup, even if a later layer fails. Final success with one leaves the backend as sole closer.

This is backend-neutral. Any provider that imports, duplicates-and-consumes, registers, or wraps a native resource in stages can use the same contract; it does not encode an NSS type or operation.