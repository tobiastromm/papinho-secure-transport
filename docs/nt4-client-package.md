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
