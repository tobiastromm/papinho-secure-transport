# NT4 client package

Phase 6.A prepares `build/nt4-validation/client` for manual copying to Windows NT 4.0 SP6. This does not complete or substitute the NT4 validation gate.

The package contains VC6 Win32 tests, the canonical repository NSS/NSPR runtime, simple NT4 `cmd.exe` BAT files, and artificial TEST-ONLY client fixtures. The fixture private key is deliberately public test material, is not a user secret, is not tracked by Git, and must never be used outside this validation.

The sibling `build/nt4-validation/server-fixture` remains on the modern host. Start the supplied `tests/nt4_tls_server.py` twice on host-reachable interfaces, for example:

```text
python tests\nt4_tls_server.py 0.0.0.0 8442 server.pem server.key ca.pem 12 fixture/1 required
python tests\nt4_tls_server.py 0.0.0.0 8443 server.pem server.key ca.pem 13 fixture/1 required
```

Use the full paths under `build\nt4-validation\server-fixture` when starting outside that directory. Permit only the needed local test ports through the host firewall. `HOST` is the modern host IP reachable by NT4; `HOSTNAME` remains `localhost`, matching the artificial certificate.

On NT4, copy the complete `client` directory, enter it using `cd`, and run `run_smoke.bat`, `run_tls12.bat HOST 8442 localhost`, or `run_tls13.bat HOST 8443 localhost`. See the package `README-NT4.txt` for logging and `run_all.bat`.

The first NT4 SP6 execution showed that `exit /b` did not terminate these wrappers reliably. The package BAT files therefore use only explicit labels and `goto`, finish naturally at end-of-file, establish success with `ver >nul`, and establish failure with the NT-compatible `verify other 2>nul` idiom. They do not invoke `exit`, so running a BAT directly cannot close the user's command window.

The first real run passed the four smoke executables and proved TLS 1.3, mTLS, and ALPN. Its `READ=0` field means zero received application bytes, so it does not yet prove bidirectional secure I/O. Preserve the next run's full output and error level; a successful echo round trip from the current integration executable must report `WRITE=25 READ=25 CONTENT_MATCH=1`.

The Phase 6.B package contains the rebuilt VC6 integration executable and deterministic echo fixture behavior. Modern-host TLS 1.2 and TLS 1.3 validation produced `WRITE=25 READ=25 CONTENT_MATCH=1`, while the server produced `RECV=25 SEND=25 CONTENT_MATCH=True`. A new real NT4 run is still required; Phase 6 remains in progress.
Phase 6.B-R1 is diagnostic only. The second real NT4 execution proved that the server received, validated, and sent all 25 bytes, while the client still reported READ=0 CONTENT_MATCH=0. This disproves the fixture-only hypothesis; no PST behavior has been changed yet.

For the next NT4 run, start the TLS 1.3 server normally and execute run_tls13_diag.bat HOST PORT HOSTNAME. Send back tls13diag-client.log, tls13diag-backend.log, and the complete server output. The backend log includes SSL_DataPending, PR_Poll input/output flags and classification, plus PR_Read return, PR_GetError, would-block status, and generated PST classification.
Phase 6.B-R2 adds timing without changing readiness behavior. The client log now records elapsed milliseconds per READ_STEP, duration of each wait, and READ_LOOP_ELAPSED_MS. The backend log records duration_ms for every PR_Poll. Run run_tls13_diag.bat as before and return both logs; these values will determine whether persistent WRITE-only readiness consumes all 200 steps before READ becomes ready.

## Phase 6.C final negative gates

The package adds `bad-ca.der`, an artificial TEST-ONLY self-signed CA unrelated to the normal server fixture. No private key for this CA is shipped. Start a fresh TLS 1.3 server for each gate:

```text
python tests\nt4_tls_server.py 0.0.0.0 8443 build\nt4-validation\server-fixture\server.pem build\nt4-validation\server-fixture\server.key build\nt4-validation\server-fixture\ca.pem 13 fixture/1 required
```

From the copied client directory on NT4, run:

```text
run_untrusted_ca.bat HOST 8443 localhost
run_missing_client_credential.bat HOST 8443 localhost
```

Restart the one-connection server between commands. The first runner uses `bad-ca.der` while retaining the normal client certificate. It passes only for normalized `PST_RESULT_AUTH_FAILURE`. The second passes `- -` in the certificate and private-key positions, so credentials are absent by explicit harness configuration without deleting package files. It accepts a normalized protocol failure during handshake, or a normalized protocol/transport failure after the server rejects the credential-free peer, with the additional required evidence `WRITE=25 READ=0 CONTENT_MATCH=0`. Any unexpected successful connection or unrelated failure makes the BAT return nonzero.

## Phase 6 closure runner history

The closure audit required repeated real TLS cycles in one VC6 process, bounded shutdown, and peer-snapshot validity after connection destruction. The Phase 6.D runner was created for that purpose and subsequently passed on real NT4.

## Phase 6.D final lifecycle package

The package now contains `test_nt4_lifecycle_integration.exe` and the NT4-compatible `run_lifecycle.bat`. The executable performs three complete public-API TLS 1.3 mTLS/required-ALPN cycles in one process. Each cycle creates and releases its runtime, creates a fresh transport and connection, validates the 25-byte echo, completes bounded incremental shutdown, destroys the connection, and then validates the independently owned peer snapshot.

Start the modern fixture from the repository root with:

```text
python tests\nt4_lifecycle_server.py 0.0.0.0 8443 build\nt4-validation\server-fixture\server.pem build\nt4-validation\server-fixture\server.key build\nt4-validation\server-fixture\ca.pem 13 fixture/1 required 3
```

On NT4, from the copied package directory, run:

```text
run_lifecycle.bat HOST 8443 localhost
```

The Windows 10 validation passed all three cycles with `WRITE=25 READ=25 CONTENT_MATCH=1 SHUTDOWN_COMPLETE=1 SNAPSHOT_AFTER_DESTROY=1`. The same gate subsequently passed on real NT4 for all three cycles. Phase 6 is complete; the package remains reproducible validation evidence.

## Phase 7.B targeted failure package

The ignored client package now includes test_connection_failures.exe and the NT4-compatible run_failure_regression.bat. This is package preparation, not NT4 evidence.

Modern server, once per mode:

    python tests\connection_failure_server.py 0.0.0.0 PORT MODE build\nt4-validation\server-fixture\server.pem build\nt4-validation\server-fixture\server.key build\nt4-validation\server-fixture\ca.pem 13 fixture/1 required

From the copied NT4 package directory:

    run_failure_regression.bat HOST PORT localhost data_then_close
    run_failure_regression.bat HOST PORT localhost clean_close
    run_failure_regression.bat HOST PORT localhost abrupt_close

Also rerun run_tls13.bat against the normal TLS 1.3 echo fixture. Return complete client and server output. The first two modes passed on the modern host. Abrupt deliberately remains failing because the preserved NSS returns zero from PR_Read for FIN without close_notify and the backend reports clean CLOSED instead of TRUNCATED. Do not report abrupt NT4 PASS unless output actually shows the required distinction.

## Phase 7.B NT4 timeline diagnostic package

The first real targeted run showed a long client-side delay after the server completed, but module provenance alone could not identify the operation. The package now contains run_failure_diag.bat and an instrumented test_connection_failures.exe. Run only clean_close first:

    run_failure_diag.bat HOST 8443 localhost clean_close

The BAT deletes prior failure-client.log, failure-backend.log and failure-modules.log before execution. Return all three new files and complete server output. Do not run data_then_close or abrupt_close again until the clean_close timeline identifies the delayed operation.
