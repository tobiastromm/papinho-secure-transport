# VC6 CRT M5 exact-final-byte recertification

This internal evidence record is separate from the M4 pre-freeze candidate
certification. M4 validated the candidate integration path; M5-RECERT validated
the exact final 0.6.2 package bytes frozen at release commit
`dcda311959b6cce073c64b6b9ce5b3635b8cd67b`.

## Exact package under test

The final Browser integration consumed only:

```text
papinho-secure-transport-0.6.2-win32-x86-vc6-retrozilla-nss-md.zip
SHA256=9dfef95b44ba2bc75b4ac6dc276e0b752037ccdb18f73852ddfcd65d7c3c7b9f
internal SDK hashes checked=47
internal SDK hash failures=0
```

No PST build-tree library, `/ML` package, earlier M2/M4 candidate, or rebuilt
substitute was used.

## Real Windows NT 4.0 SP6 x86

Owner-executed final-byte validation passed for both `/ML` and `/MD`:

- public contract probe, including acceptance of graceful feature values 0, 1
  and 2 and rejection of the unknown value as
  `PST_RESULT_INVALID_ARGUMENT` (`1`);
- CLIENT TLS 1.3;
- SERVER TLS 1.3 with required mTLS;
- 25-byte encrypted read/write and content match;
- reciprocal graceful shutdown;
- data-preserving strict truncation after raw EOF;
- NSS `PR_Poll` readiness authority.

The supplied backend traces independently show successful TLS 1.3 handshakes,
25-byte transfers and reciprocal `close_notify` for both CRT variants. Their
truncation traces preserve the 25-byte read and then classify EOF without
`close_notify` as truncated. No replacement `MSVCRT.DLL` was installed; NT4's
existing System32 runtime was used.

```text
FINAL_NT4_ML_CONTRACT=PASS
FINAL_NT4_MD_CONTRACT=PASS
FINAL_NT4_ML_CLIENT_TLS13=PASS
FINAL_NT4_MD_CLIENT_TLS13=PASS
FINAL_NT4_ML_SERVER_TLS13_MTLS=PASS
FINAL_NT4_MD_SERVER_TLS13_MTLS=PASS
FINAL_NT4_ML_TRUNCATION=PASS
FINAL_NT4_MD_TRUNCATION=PASS
FINAL_NT4_ML_PR_POLL=PASS
FINAL_NT4_MD_PR_POLL=PASS
FINAL_NT4_ML_MD_PARITY=PASS
```

## Separate clean Windows machine

Owner-executed final-byte validation used VC6 `12.00.8804` x86. Both variants
passed the contract matrix and unknown-value rejection. Both passed real TLS
1.3 (`0x0304`, cipher `0x1302`), authentication result `2`, 25-byte encrypted
write/read, content match, reciprocal shutdown, SNI/expected peer name
`localhost`, and NSS `PR_Poll` readiness.

```text
FINAL_CLEAN_ML=PASS
FINAL_CLEAN_MD=PASS
FINAL_CLEAN_ML_MD_TLS_PARITY=PASS
FINAL_CLEAN_UNKNOWN_FEATURE_REJECTED=PASS
FINAL_CLEAN_NODEFAULTLIB_USED=NO
```

## Final PapinhoBrowser `/MD` integration

A new isolated Browser worktree at Browser commit
`ffa7a505c5a359f74de6e77ecdc03b3487253ef9` retained VC6 x86 `/MD` and reused
the approved M4 integration probe against the exact final SDK. The PST
integration target compiled and linked without `/NODEFAULTLIB`, a PST/Browser
CRT conflict, or a cross-CRT free workaround. The executable imports
`MSVCRT.dll`.

The official TLS fixture and Browser integration probe produced:

```text
SNI OBSERVED=localhost EXPECTED=localhost MATCH=True
CLIENT AUTH=True ALPN=None
IO RECV=25 SEND=25 CONTENT_MATCH=True
SHUTDOWN RECIPROCAL_CLOSE_NOTIFY=True

BROWSER_PST_PEER TLS=772 AUTH=2 CIPHER=4866
BROWSER_PST_MD_REAL_TLS=PASS PROVIDER=retrozilla-nss TLS=0x0304
WRITE=25 READ=25 CONTENT_MATCH=1 SHUTDOWN=RECIPROCAL
```

The real Browser GUI executable also started, exposed its top-level window,
navigated through its existing event-loop path, captured the rendered window,
and closed normally. The test-only integration remained isolated and did not
change Browser production networking. The Browser still owns its event loop;
the consumer-created connected socket is transferred exactly once to PST; and
exact provider selection permits no post-binding provider switch.

```text
FINAL_BROWSER_PST_MD_PACKAGE_HASH_MATCH=PASS
FINAL_BROWSER_PST_MD_PACKAGE_ONLY=YES
FINAL_BROWSER_PST_TARGET=win32-x86-vc6-retrozilla-nss-md
FINAL_BROWSER_CRT_REMAINS_MD=YES
FINAL_BROWSER_MD_COMPILE=PASS
FINAL_BROWSER_MD_LINK=PASS
FINAL_BROWSER_LIBC_CONFLICT=NO
FINAL_BROWSER_NODEFAULTLIB_USED=NO
FINAL_BROWSER_STARTUP=PASS
FINAL_BROWSER_PST_RUNTIME_LOAD=PASS
FINAL_BROWSER_TLS13=PASS
FINAL_BROWSER_CONTENT_RECEIVED=PASS
FINAL_BROWSER_EVENT_LOOP_OWNERSHIP=PASS
FINAL_BROWSER_PST_OWNERSHIP=PASS
FINAL_BROWSER_POST_BIND_SWITCHES=0
FINAL_BROWSER_CROSS_CRT_FREE_WORKAROUND=NO
```

## Security and package immutability

Eight supplied backend log files were scanned. Six contain reciprocal-close
completion and two contain the expected truncation terminal state. Results:

```text
LOG_SECRET_HITS=0
PRIVATE_KEY_LOG_HITS=0
APPLICATION_PAYLOAD_LOG_HITS=0
PERSONAL_PATH_HITS=0
```

`tools/stage-release-source.ps1` explicitly excludes `docs/codex/`, and
package validation rejects any leak of that directory. This evidence record
therefore does not enter canonical source or SDK package bytes. No package was
regenerated.

The previously completed same-state and cross-EOL reproduction results remain:

```text
FINAL_PACKAGE_REPRODUCTION=PASS
FINAL_PACKAGE_REPRODUCTION_ACROSS_EOL_STATES=PASS
EOL_REPRO_BAD_COUNT=0
```

The seven canonical hashes were recalculated after this evidence record was
added and remained unchanged:

```text
SOURCE=7e5691f4258caeda38de185aa7c687dc736dc062f5bcdbf765a9c945033d36fa
VC6_ML=0b1a9c186c6c0b6b5dfee7f8fa4ef908cd6737638f7337adbceebfb3e1d60c7b
VC6_MD=9dfef95b44ba2bc75b4ac6dc276e0b752037ccdb18f73852ddfcd65d7c3c7b9f
SCHANNEL=44610dcf040a8c880e2df5e651bca2c9c3d0e613639ba0c924494cb8b336ac20
OPENSSL=e4ad8774b74d45474ddc394f788a9b7057857f4422ff1feeb7adeffbd81092d8
COMBINED=b1e533b55355900d0656b1d7a8b0c39092a06c217f47017b933fb539f6847efd
CHECKSUM_FILE=50438b03b556fe974293bc64e50f50d18f8c3d97324bd4e99a05c7ed876707c0
```

## Closure

```text
FINAL_EXTERNAL_RECERTIFICATION=PASS
FINAL_PACKAGE_HASHES_UNCHANGED_AFTER_RECERT_EVIDENCE=YES
M5_A_FREEZE_AND_FINAL_VALIDATION=PASS
READY_FOR_PUBLICATION_AUTHORIZATION=YES
```
