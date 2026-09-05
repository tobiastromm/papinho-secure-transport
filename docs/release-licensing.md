<!-- SPDX-License-Identifier: MPL-2.0 -->

# Release licensing audit

This is a technical inventory, not legal advice.

## Applied project license

PapinhoSecureTransport project-authored files are licensed under the standard Mozilla Public License 2.0 without an Exhibit B incompatibility notice. The authoritative unmodified text is in the root LICENSE file; short SPDX identifiers identify covered project files.

The authority audit found only Tobias Tromm and tobiastromm with the same email identity in the history of the project-owned trees, and no external notices outside third_party. No known authority blocker remains. This is a repository-evidence conclusion, not legal advice.

The four static SDKs preserve file-level source availability through papinho-secure-transport-0.4.0-src.zip. Third-party files retain their independent terms and are not relicensed as PST code.

## Third-party classification

| Component | Evidence | Package treatment | Release status |
|---|---|---|---|
| RetroZilla/NSS 3.42 Beta/NSPR 4.7.7 lineage | `third_party/retrozilla-nss/licenses/`, `PROVENANCE.md`, source/binary manifests and ordered patch | legacy SDK carries preserved license evidence and runtime binaries | SOURCE/NOTICE PRESERVED |
| OpenSSL 3.5.8 LTS | Apache-2.0 text, source/prebuilt manifests and provenance | OpenSSL SDK carries license text, import libraries and two runtime DLLs | LICENSE/PROVENANCE PRESERVED |
| Windows Schannel/CryptoAPI/Winsock/CNG and CRT/UCRT | supplied by Windows or Microsoft redistributable/toolchain | not copied by PST staging | NOT REDISTRIBUTED |

RetroZilla/NSS/NSPR files contain historical MPL and tri-license notices. Preserving texts is necessary but is not itself a conclusion about every source or modification obligation. The fail-closed RNG patch and source/build records remain linked from provenance.

Modern `/MD` consumers require the appropriate Microsoft Visual C++ runtime. PST staging does not copy Microsoft installers or DLLs; deployment must follow Microsoft's applicable terms. Windows system DLLs must not be repackaged as PST files.
