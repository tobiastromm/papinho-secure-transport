# Third-party notices

This summarizes redistributed third-party material. It does not replace authoritative license texts and is not legal advice.

## RetroZilla NSS / Mozilla NSS / NSPR lineage

- Identity: RetroZilla-derived NSS 3.42 Beta and NSPR 4.7.7 legacy Win32 build.
- Origin, snapshot, modifications, fail-closed RNG patch, build process and hashes: `third_party/retrozilla-nss/PROVENANCE.md` and manifests.
- License evidence: `third_party/retrozilla-nss/licenses/RetroZilla-LICENSE.txt`, `RetroZilla-LEGAL.txt`, `NSS-MPL-2.0.txt`, and `NSPR-license-evidence.h`.
- Redistribution: selected runtime binaries accompany only the legacy NSS SDK candidate. Source/modification obligations require final review.

## OpenSSL

- Identity: OpenSSL 3.5.8 LTS.
- Origin/build: official archive SHA-256 `a8f84a39918ec6415ce765d9b429d313ba97b8143169c172e734b9514464f5b2`; no local source patch; recorded suite result 4137 tests in 346 files, PASS.
- Full provenance/hashes: `third_party/openssl/PROVENANCE.md` and manifests.
- License: Apache License 2.0, preserved at `third_party/openssl/LICENSE.txt` and `third_party/openssl/licenses/LICENSE-APACHE-2.0.txt`.
- Redistribution: import libraries and `libssl-3-x64.dll`/`libcrypto-3-x64.dll` accompany only the OpenSSL SDK candidate.

## Microsoft Windows and Visual C++ runtime

Schannel, CryptoAPI, CNG, Winsock, Windows system DLLs, and Microsoft CRT/UCRT components are not redistributed by PST technical staging. They are platform/toolchain prerequisites governed by their supplier's terms.
