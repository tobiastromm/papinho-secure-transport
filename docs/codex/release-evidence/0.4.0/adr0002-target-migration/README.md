# ADR-0002 target migration evidence

This directory binds the pre-release 0.4.0 package-name migration to the accepted PapinhoEngineering ADR-0002 revision 2. Historical validation evidence retains the identifiers that were actually tested; `target-crosswalk.txt` maps them to the canonical IDs.

The package ZIPs changed because paths, manifests, documentation and checksums changed. `binary-identity-manifest.txt` records the old and new payload hashes. All PST libraries, OpenSSL import libraries/DLLs, NSS/NSPR DLLs and CHK files, and public headers compare byte-identically. The preserved NT4 validation executable remains SHA-256 `9dc2220a869c71ff2b1ef85a27c77a77b280840be5a9a3ffe31b211b062cfd9c`.

`package-validation.log` records five-package structural validation and four extracted-package consumer compile/link/runtime passes. These facts permit inheritance of the existing NT4 and x64 TLS runtime evidence without claiming a new clean-machine execution.