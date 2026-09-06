# OpenSSL 3.5.8 dependency

This directory preserves the exact official OpenSSL 3.5.8 LTS source used by the PapinhoSecureTransport OpenSSL provider target. The canonical archive is `source/openssl-3.5.8.tar.gz`; verify it against `source/MANIFEST.sha256` before extraction. `LICENSE.txt` is the unmodified Apache-2.0 license from that release, also retained as `licenses/LICENSE-APACHE-2.0.txt`.

The Windows x64 dependency is staged under `prebuilt/win32-x64-msvc-19.51-openssl3/3.5.8`. It contains generated public headers, MSVC import libraries, and only the required shared runtime DLLs. Build trees, PDBs, engines, applications, legacy provider, and FIPS provider are not staged. Hashes identify this build; they are not a claim that independent builds are byte-reproducible.

See `PROVENANCE.md` and `docs/build-openssl.md` for exact preparation and verification.
