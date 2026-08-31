# Legacy platform validation

Status: Phase 6 in progress. The mandatory Windows NT 4.0 SP6 functional gate has not yet been executed and no legacy support claim is made.

## Environment audit

The available host is Windows 10 Pro 10.0.19045 x64. No installed VirtualBox, VMware, QEMU, 86Box, PCem, or DOSBox-X command and no NT4 VM configuration/disk image was found. `C:\PapinhoBuildArchive\RetroZilla-NSS-NT4-VC6-2026-08-30` contains preserved build/test evidence and artifacts, but, as recorded by the provenance audit, is not a bootable or reproducible NT4 VM. Therefore it cannot satisfy the Phase 6 requirement for execution on Windows NT 4.0 SP6.

Windows 2000 and Windows XP environments were not available and were not claimed as tested.

## Build and static compatibility evidence

The current public runtime integration executable was built by Visual C++ 6.0 (`_MSC_VER == 1200`) with `/W4`, as Win32 i386 CUI. PE metadata reports linker 6.00, OS/subsystem version 4.00, no delay-import directory, and static linkage of the application CRT (`LIBC`). Direct application dependencies are `WSOCK32.dll` and `KERNEL32.dll`.

The imported KERNEL32 functions are from the VC6-era surface and include ANSI loader/file/environment APIs. The scan found no newly introduced post-NT4-only API. This is static evidence only; loader success on NT4 remains unproven.

Canonical runtime dependencies:

- `freebl3.dll`: `nssutil3`, `nspr4`, `MSVCRT`, `KERNEL32`;
- `nspr4.dll`: `ADVAPI32`, `WSOCK32`, `WINMM`, `MSVCRT`, `KERNEL32`;
- `nss3.dll`: `nssutil3`, `plc4`, `plds4`, `nspr4`, `MSVCRT`, `KERNEL32`;
- `ssl3.dll`: `nss3`, `nssutil3`, `plc4`, `nspr4`, `MSVCRT`, `KERNEL32`;
- `softokn3.dll`: `nssutil3`, `plc4`, `plds4`, `nspr4`, `MSVCRT`, `KERNEL32`.

The normal fail-closed pair remains unchanged:

- `freebl3.dll`: `12808c651528c9e08f5ccf86af00f9061b19103c0c712d376755664f41ee474d`;
- `freebl3.chk`: `122aefebbfd76eb68352eb23d59d4caff70183b520f81b5017bf299f7b745daa`.

No failure-injection artifact is present in the runtime. No RNG implementation or fallback was changed in Phase 6.

## Mandatory NT4 matrix

All entries below remain **NOT EXECUTED** on NT4: foundation, SPI, NSS lifecycle, identity, public runtime load, server authentication, TLS 1.2, TLS 1.3, mTLS, ALPN, nonblocking progress, `PR_WOULD_BLOCK_ERROR`, SSL-descriptor `PR_Poll`, secure/partial I/O, peer snapshot lifetime, graceful shutdown, wrong hostname, untrusted CA, downgrade rejection, missing client credential, and repeated lifecycle.

The Windows 10 Phase 5 evidence cannot replace any entry in this matrix. Module paths logged on Windows 10 likewise do not prove which DLLs an NT4 process would load.

## Required next execution

Provide or start a real Windows NT 4.0 SP6 x86 VM/machine reachable from a modern local TLS fixture server. Copy the VC6-built smoke tests, public integration executable, canonical runtime DLLs, and ephemeral public certificate inputs to it. Execute the mandatory matrix through the public PST API and preserve client/server outputs plus backend trace. Only then may this document and roadmap mark Phase 6 complete.

No code correction was made in Phase 6 because no NT4 incompatibility was reproduced. Phase 7 has not started.
