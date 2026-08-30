# RetroZilla NSS/NSPR legacy assets

This directory preserves the minimum runtime candidate, source patch, license evidence, and audit manifests. It excludes the full source tree, MozillaBuild, build output, ZIPs, credential databases, and private keys.

Runtime roles:

- ssl3.dll: TLS; required.
- nss3.dll and nssutil3.dll: NSS core/utilities; required.
- softokn3.dll/chk: software PKCS #11 module and integrity file; required.
- freebl3.dll/chk: crypto module and integrity file; required; normal fail-closed variant.
- nssdbm3.dll/chk: historical DBM credential database module; required for the legacy DB layout.
- nspr4.dll, plc4.dll, plds4.dll: NSPR runtime/utilities; required.

smime3.dll is excluded because S/MIME is out of scope. sqlite3.dll is excluded because the historical path used DBM. Baseline and failure-injection binaries remain only in the external archive.
