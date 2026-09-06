# Third-party notices

PapinhoSecureTransport project-authored code is licensed under the Mozilla Public License 2.0. See LICENSE. This file identifies independently licensed material and does not replace its authoritative license texts.

## RetroZilla NSS / Mozilla NSS

The legacy SDK redistributes selected RetroZilla-derived NSS 3.42 Beta runtime files. NSS components are covered by their existing Mozilla Public License 2.0 notices. They are not relicensed as PapinhoSecureTransport code.

Exact origin, snapshot, modifications, fail-closed RNG patch, build procedure, and hashes are recorded in third_party/retrozilla-nss/PROVENANCE.md and its manifests. The corresponding 0.4.0 source package contains revision 2f274574d3c6ee8769914046920d649bbae9f81b, the patched snapshot with SHA-256 5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388, and patch 0001-win32-secure-rng-fail-closed-nt4.patch.

Preserved license/legal evidence: RetroZilla-LICENSE.txt, RetroZilla-LEGAL.txt, NSS-MPL-2.0.txt, and NSPR-license-evidence.h.

## NSPR 4.7.7

The legacy SDK redistributes nspr4.dll, plc4.dll, and plds4.dll under their historical tri-license choice: MPL-1.1 / GPL-2.0-or-later / LGPL-2.1-or-later. NSPR has not been relicensed under the PST MPL-2.0 license. The corresponding source and license evidence are preserved in the 0.4.0 source package.

## OpenSSL 3.5.8 LTS

The OpenSSL and Combined SDKs redistribute libssl-3-x64.dll, libcrypto-3-x64.dll, libssl.lib, and libcrypto.lib under Apache License 2.0. They are not relicensed under MPL-2.0. The authoritative Apache-2.0 text is included in the applicable SDK. No upstream NOTICE file exists in the prepared source tree.

The official source archive has SHA-256 a8f84a39918ec6415ce765d9b429d313ba97b8143169c172e734b9514464f5b2. Provenance, source, manifests, and hashes are preserved under third_party/openssl and in the 0.4.0 source package.

## Microsoft Windows and toolchains

Schannel, CryptoAPI, CNG, Winsock, Windows system DLLs, and Microsoft CRT/UCRT components are OS_SUPPLIED or deployment/toolchain prerequisites and are NOT_REDISTRIBUTED by these SDKs. VC6, Visual Studio Build Tools, Windows SDK, and NMAKE are build-only dependencies governed by their suppliers' terms.
