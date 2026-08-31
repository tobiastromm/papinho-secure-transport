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
