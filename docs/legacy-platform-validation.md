# Legacy platform validation

Status: Phase 6 in progress. Real Windows NT 4.0 SP6 execution has passed TLS 1.3 and TLS 1.2 mTLS/ALPN with bidirectional secure I/O, TLS 1.2 server-authentication without a client certificate, wrong-hostname rejection, and the TLS 1.3-required no-downgrade gate. The untrusted-CA and missing-client-credential gates remain pending on NT4, so no complete legacy support claim is made.

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

## First real NT4 execution evidence

On Windows NT 4.0 SP6, `run_smoke.bat` executed all four VC6 test programs successfully: foundation, backend SPI, NSS backend, and identity. It printed `PAPINHOSECURETRANSPORT NT4 SMOKE TESTS PASS`. The original wrapper then reached failure labels because NT4 `cmd.exe` did not handle its `exit /b` termination as expected; this was a BAT compatibility defect after the executable results, not a failure of those four tests.

The public runtime integration executable also completed a real TLS 1.3 handshake and printed `BACKEND=retrozilla-nss TLS=0x0304 WRITE=25 READ=0 ALPN=9 AUTH=2`. The modern fixture independently recorded `CLIENT ('172.16.0.11', 2797) AUTH=True ALPN=fixture/1`. This proves the RetroZilla NSS backend path, TLS 1.3 negotiation, client authentication, and ALPN on NT4.

`READ=0` is the integration test's `rd` byte accumulator, not a result code or close-state field. It means that this execution received zero application-data bytes. The current source requires both `WRITE=25` and `READ=25`, with equal payloads, for `ok` to remain true and the process to return zero. Consequently, secure bidirectional application I/O is not yet proved by this run. The reported BAT PASS and the source's expected exit status for `READ=0` are inconsistent and must be captured again after the BAT compatibility correction.

All six package wrappers were changed to avoid `exit /b`. Success now reaches EOF with a zero status established by `ver >nul`; usage and failure paths reach EOF with a nonzero status established by the NT-compatible `verify other 2>nul` idiom. Explicit `goto` paths prevent success from falling through into error labels and do not close the user's command window.

Phase 6.B initially hardened the fixture shutdown ordering: it now accumulates and validates exactly 25 expected bytes, sends the deterministic echo, records RECV, SEND, and CONTENT_MATCH, then waits at most 30 seconds for client shutdown. A second real NT4 run nevertheless produced WRITE=25 READ=0 CONTENT_MATCH=0 while the server independently recorded RECV=25 SEND=25 CONTENT_MATCH=True. That result disproves the earlier fixture-only hypothesis and confirms that the echo was sent.

Phase 6.B-R1 adds diagnostic-only tracing without changing public API, SPI, ownership, readiness behavior, or result classification. The current backend maps PR_POLL_ERR or PR_POLL_NVAL to transport failure and maps any PR_POLL_HUP to PST_RESULT_CLOSED, even when PR_POLL_READ is also present. pst_connection_wait returns that result but does not itself change connection state; the integration loop stops when wait is not PST_RESULT_OK, so PR_Read might not be retried. The NSS lineage provides SSL_DataPending and its SSL poll layer uses it to report buffered plaintext readiness. The diagnostic trace records pending bytes, requested and returned poll flags, poll classification, PR_Read return/error/would-block classification, and public read totals.

On the Windows 10 validation host, the diagnostic VC6 executable passed TLS 1.3. One observed package run first returned PR_WOULD_BLOCK_ERROR from PR_Read, then PR_Poll reported READ|WRITE without HUP, and the next PR_Read returned 25 bytes. Final evidence was WRITE=25 READ=25 CONTENT_MATCH=1 ALPN=9 AUTH=2 with server RECV=25 SEND=25 CONTENT_MATCH=True. This baseline does not prove which NT4 branch occurs; a new diagnostic NT4 run is required.
The real NT4 R1 logs then showed 200 consecutive PR_Read would-block results. Every application-read PR_Poll requested READ|WRITE and returned WRITE only, with no READ, ERR, HUP, NVAL, timeout, or SSL_DataPending bytes. This rejects the READ|HUP hypothesis for that execution and supports a possible WRITE-readiness spin, but the R1 logs contain no timestamps.

Source audit shows that application-data PR_Read would-block is conservatively mapped by the PST NSS backend to NEED_READ_WRITE. In the NSS SSL poll layer, after the first handshake is complete and SSL_DataPending is zero, READ|WRITE is passed to the lower descriptor. A writable lower socket may therefore make PR_Poll return immediately with WRITE even though the read operation cannot advance. SSL_DataPending reports only plaintext already buffered in the SSL receive buffer; zero does not state whether ciphertext is queued in the transport or may arrive later.

Phase 6.B-R2 adds GetTickCount-based diagnostic timing, available on NT4: elapsed milliseconds for every read step, duration for every PR_Poll, and total read-loop elapsed time. Windows 10 reached READ=25 in one read step and 0 measured milliseconds in the captured run. No readiness behavior has been changed; the updated NT4 package is intended to measure whether its 200 WRITE-only polls consume the budget immediately.

Phase 6.B-R2A then supplied a shared process-relative monotonic timeline. The returned NT4 evidence proved that approximately 200 PR_Read would-block / immediate WRITE-only PR_Poll cycles consumed only tens of milliseconds. The root cause is therefore confirmed: conservative NEED_READ_WRITE plus level-triggered WRITE readiness repeatedly reported readiness without useful progress for the pending application read.

Phase 6.B-R3 adds an internal backend-neutral progress guard. After an auxiliary-only readiness event, a retry with zero transferred bytes, unchanged operation and unchanged interest suppresses that auxiliary bit for the following wait. The primary operation interest remains enabled. A timeout re-enables the auxiliary bit, and any byte progress, completion, interest change, handshake, or shutdown resets the guard. This preserves legitimate reads that require WRITE and keeps PR_Poll on the SSL descriptor. Windows 10 VC6 /W4 validation passed deterministic WRITE-only/no-progress, WRITE-required/progress, and READ|WRITE/progress cases, plus TLS 1.2 and TLS 1.3 mTLS/ALPN echoes with WRITE=25 READ=25 CONTENT_MATCH=1. This does not claim that NT4 is fixed until the updated package is executed there.

## Remaining mandatory NT4 matrix

The following are now proved on NT4: foundation, SPI, NSS lifecycle, identity, public runtime execution, TLS 1.3 and TLS 1.2 mTLS/ALPN, secure bidirectional I/O, TLS 1.2 server-authentication-only mode, wrong-hostname rejection, and TLS 1.3-required rejection of a TLS 1.2-only server. The two remaining final negative gates are untrusted CA and missing client credential against an mTLS-required server.

The Windows 10 Phase 5 evidence cannot replace any entry in this matrix. Module paths logged on Windows 10 likewise do not prove which DLLs an NT4 process would load.

## Required next execution

Copy the Phase 6.C package to Windows NT 4.0 SP6. Start a fresh TLS 1.3 `fixture/1` server with client certificates required for each case, then run `run_untrusted_ca.bat HOST PORT localhost` and `run_missing_client_credential.bat HOST PORT localhost`. Preserve each BAT output, generated client log, module trace, server output, and errorlevel. Both BATs must print their explicit negative-gate PASS line and return zero before Phase 6 can be considered complete.

Phase 6.B-R3 changes only the internal, backend-neutral wait/progress orchestration in the PST core. The public API, SPI version and layout, NSS backend readiness mapping, TLS policy, trust, credentials, RNG, and HUP handling remain unchanged. Phase 7 has not started.
