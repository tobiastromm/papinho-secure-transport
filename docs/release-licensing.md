# Release licensing audit

This is a technical inventory, not legal advice.

## Blocking project decision

PST has no root `LICENSE`, no selected SPDX identifier, and no package license declaration. Public headers do not grant a project-wide license. The README says that the project license is under review. Therefore public redistribution of a PST source or binary release is **BLOCKED ON PST LICENSE DECISION**.

The project owner must explicitly select and approve a license. Options to review include permissive licenses such as MIT, BSD-2-Clause, BSD-3-Clause, or Apache-2.0, and reciprocal licenses such as MPL-2.0. The choice affects notice, patent, source/modification, and compatibility expectations; no option is selected here.

After selection, add the authoritative root license, copyright holder/year policy, header policy if desired, README declaration, package metadata, and a compatibility review against every redistributed dependency.

## Third-party classification

| Component | Evidence | Package treatment | Status before public release |
|---|---|---|---|
| RetroZilla/NSS 3.42 Beta/NSPR 4.7.7 lineage | `third_party/retrozilla-nss/licenses/`, `PROVENANCE.md`, source/binary manifests and ordered patch | legacy SDK carries preserved license evidence and runtime binaries | REVIEW REQUIRED |
| OpenSSL 3.5.8 LTS | Apache-2.0 text, source/prebuilt manifests and provenance | OpenSSL SDK carries license text, import libraries and two runtime DLLs | VERIFIED evidence; final combined-distribution review required |
| Windows Schannel/CryptoAPI/Winsock/CNG and CRT/UCRT | supplied by Windows or Microsoft redistributable/toolchain | not copied by PST staging | NOT REDISTRIBUTED |

RetroZilla/NSS/NSPR files contain historical MPL and tri-license notices. Preserving texts is necessary but is not itself a conclusion about every source or modification obligation. The fail-closed RNG patch and source/build records remain linked from provenance.

Modern `/MD` consumers require the appropriate Microsoft Visual C++ runtime. PST staging does not copy Microsoft installers or DLLs; deployment must follow Microsoft's applicable terms. Windows system DLLs must not be repackaged as PST files.
