# VC6 CRT M4 external validation

This internal evidence record summarizes owner-executed M4 validation of the
explicit VC6 `/ML` and `/MD` RetroZilla NSS package candidates. It records the
external results without freezing final 0.6.2 package hashes or changing the
published 0.6.1 baseline.

## Evidence corrections retained in the audit trail

The initial NT4 CLIENT TLS 1.3 run failed with NSS native error `-12276`
(`SSL_ERROR_BAD_CERT_DOMAIN`). The fixture invocation supplied an incorrect
expected peer name. This was classified
`FIXTURE_CONFIGURATION_DEFECT_EXPECTED_PEER_NAME_MISMATCH`; the corrected run
with expected peer name `localhost` passed. It was not a PST, package, runtime,
or CRT regression.

The initial NT4 SERVER runs printed `READY` but accepted connections only on
loopback because the validation harness bound `INADDR_LOOPBACK`. Remote clients
therefore received connection refused. This was classified
`VALIDATION_KIT_DEFECT_LOOPBACK_ONLY_LISTENER`. The R1 harness binds
`INADDR_ANY`, verifies the consumer-owned listener before printing `READY`, and
then passes the accepted transport to PST. Corrected TLS 1.2, TLS 1.3 required
mTLS, and truncation runs passed for both CRT variants.

An earlier NT4 report of `GRACEFUL_UNKNOWN_RESULT=-1` came from stale or
different harness evidence. The canonical R1 contract probe prints the direct
return from `pst_connection_create`; unknown `PST_FEATURE_*` input returns
`PST_RESULT_INVALID_ARGUMENT`, whose public numeric value is `1`. The R1
`contract.exe` is byte-identical between the NT4 and clean-machine bundles for
each CRT variant:

```text
ML  3aafa114a64d301e880d6cd8dac64145b40ad4c391d5c38ee0e5dded2fe90de0
MD  d10d6e4485b987d9d92dc96d8d979dd16c3341dce1001f3ffd78cc1a82bcd424
```

## Real Windows NT 4.0 SP6 x86

Both `/ML` and `/MD` passed the public contract probe, CLIENT TLS 1.2 and TLS
1.3, SERVER TLS 1.2, SERVER TLS 1.3 with required mTLS, 25-byte encrypted I/O,
content matching, reciprocal graceful shutdown, strict truncation after
preserving delivered data, and NSS `PR_Poll` readiness authority. NSS SERVER
ALPN remains unadvertised and the TLS 1.3 mTLS gate does not require it.

The existing NT4 `System32` `MSVCRT.DLL` satisfied the `/MD` runtime dependency.
No replacement CRT was installed or redistributed.

The supplied backend evidence contains nine positive reciprocal-close traces
and two truncation traces. Every trace completes the handshake and reads 25
bytes. Positive CLIENT/SERVER traces write 25 bytes and receive reciprocal
`close_notify`; truncation traces preserve the 25-byte read before classifying
raw EOF as truncated.

## Separate clean Windows machine

The external machine used VC6 compiler `12.00.8804` x86. Both package-only CRT
variants passed the public contract probe and rejected the unknown feature
value with `PST_RESULT_INVALID_ARGUMENT`. Both passed real TLS 1.3 with TLS
version `0x0304`, cipher `0x1302`, authentication result `2`, 25-byte encrypted
write/read, content match, reciprocal shutdown, and NSS `PR_Poll` readiness.

The R1 transfer manifest was validated by hashing every referenced file:

```text
ENTRIES=63
FILES_CHECKED=63
BAD_COUNT=0
TRANSFER-SHA256SUMS.txt=3c96d3714d51a8c201701f76ea97e074f09683906ac238bd7198fd0cb5fe23ab
```

No `/NODEFAULTLIB` workaround was used and no ML/MD TLS or lifecycle semantic
divergence was observed.

## Real PapinhoBrowser `/MD` integration

An isolated Browser worktree preserved the existing VC6 `/MD` configuration and
consumed only the extracted
`win32-x86-vc6-retrozilla-nss-md` candidate. Compilation, linking, startup and
runtime loading passed without `/NODEFAULTLIB`, a CRT conflict, cross-CRT free
workarounds, or a provider switch after binding. A controlled real TLS 1.3 run
reported provider `retrozilla-nss`, 25-byte write/read, content match and
reciprocal shutdown. Browser event-loop ownership and PST transport ownership
remained unchanged. No Browser commit was pushed.

## Security, immutability and scope

Backend-log scans reported:

```text
LOG_SECRET_HITS=0
PRIVATE_KEY_LOG_HITS=0
APPLICATION_PAYLOAD_LOG_HITS=0
PERSONAL_PATH_HITS=0
```

The published historical artifact remains byte-identical:

```text
papinho-secure-transport-0.6.1-win32-x86-vc6-retrozilla-nss.zip
SHA256=ecf221fd95a99ea0d5bd59775127814e13767ac461bc0624350c9b10d4a4f9a9
```

M4 made no PST production, public API, SPI, or third-party change. It does not
freeze final 0.6.2 hashes, regenerate final packages, create a tag or release,
or start M5. Existing TLS-after-plaintext evidence remains applicable because
the CRT-variant path did not change that behavior.

## Closure

```text
PAPINHOSECURETRANSPORT_VC6_CRT_M4=PASS
EXTERNAL_ML_MD_TLS_PARITY=PASS
EXTERNAL_ML_MD_LIFECYCLE_PARITY=PASS
EXTERNAL_FIRST_FUNCTIONAL_DIVERGENCE=NONE
REAL_NT4_ML=PASS
REAL_NT4_MD=PASS
CLEAN_MACHINE_ML=PASS
CLEAN_MACHINE_MD=PASS
REAL_BROWSER_MD_INTEGRATION=PASS
FINAL_062_HASHES_FROZEN=NO
```
