<!-- SPDX-License-Identifier: MPL-2.0 -->

# MPL 2.0 license compatibility audit

Audit date: 2026-09-04

Decision applied after owner approval: PapinhoSecureTransport project-authored files are licensed under Mozilla Public License 2.0 (`MPL-2.0`). This is a technical compatibility audit, not legal advice and not an application of that license.

## Conclusion

**CONFIRMED WITH CONDITIONS.** MPL-2.0 is technically compatible with the project's static-only four-SDK release model. Static linking does not, by itself, require unrelated application files to be licensed under MPL-2.0. The MPL obligations remain file-scoped: recipients of executable form must be told how to obtain the corresponding Source Code Form of MPL-covered files, and modified covered files remain under MPL-2.0.

Phase 9.E-L2 subsequently satisfied the recorded conditions: the authority audit found no external contributor identity, the root license and project-owned SPDX notices were applied, source/notices were staged, and third-party terms remained separate.

## Recommended MPL variant

Use the standard unmodified MPL 2.0 text and do **not** add Exhibit B (`Incompatible With Secondary Licenses`). This preserves the standard Section 3.3 route for combining covered files into an actual Larger Work with GPL, LGPL, or AGPL code. Exhibit B would disable that route and unnecessarily reduce downstream compatibility.

This recommendation requires an explicit owner decision. It has not been applied by this audit.

## Project-owned scope

The intended MPL-covered scope is project-authored content under these trees:

| Tree | Files observed | Intended treatment |
|---|---:|---|
| `include/` | 2 | MPL-2.0 after owner approval |
| `src/` | 24 | MPL-2.0 after owner approval |
| `tools/` | 8 | MPL-2.0 after owner approval |
| `tests/` | 46 | MPL-2.0 after owner approval |
| `examples/` | 7 | MPL-2.0 after owner approval |
| `docs/` | 50 | MPL-2.0 after owner approval |
| `packaging/` | 1 | MPL-2.0 after owner approval |

`third_party/` is expressly excluded from relicensing. Generated build and staging outputs are governed by the licenses of their inputs and components, not assigned a blanket PST license.

A repository scan found no conflicting third-party license markers in the intended project-owned trees. Before application, the owner must nevertheless confirm authorship or sufficient licensing authority for all contributions that will be designated MPL-2.0.

## Static linking and consumer applications

Mozilla's MPL FAQ expressly permits an MPL library to be statically linked into a larger proprietary work. The consumer's separate new files do not become MPL-covered merely because they link the PST static library. The consumer must still comply with MPL-2.0 for PST covered files: preserve notices, disclose modifications to those files, and make their corresponding source available when distributing executable form.

The four candidate SDKs therefore remain viable:

| Candidate SDK | PST form | Principal external boundary | Compatibility result |
|---|---|---|---|
| Windows NT 4.0 x86 / VC6 / RetroZilla NSS | static PST library plus runtime DLLs | NSS/NSPR notices and corresponding source | compatible with conditions |
| Windows x64 / modern MSVC / Schannel | static PST library | Windows system facilities are not redistributed | compatible with conditions |
| Windows x64 / modern MSVC / OpenSSL 3.5.8 | static PST library plus OpenSSL runtime DLLs/import libraries | Apache-2.0 license and notices remain separate | compatible with conditions |
| Windows x64 / modern MSVC / Schannel + OpenSSL 3.5.8 | combined static PST library plus OpenSSL runtime DLLs/import libraries | Windows remains OS-supplied; OpenSSL remains Apache-2.0 | compatible with conditions; optional, not default |

A binary SDK must contain a conspicuous source-availability notice. The corresponding source must be available at the same time and by a reasonable means, at no more than the cost of distribution. The release source package must correspond exactly to the shipped PST binaries and include the scripts needed to modify and rebuild covered code.

## RetroZilla NSS and NSPR inventory

The staged legacy runtime matches the canonical repository runtime byte-for-byte. The canonical patched source snapshot is:

- revision: `2f274574d3c6ee8769914046920d649bbae9f81b`
- archive: `third_party/retrozilla-nss/source/retrozilla-2f274574d3c6ee8769914046920d649bbae9f81b-patched.zip`
- SHA-256: `5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388`

| Staged file | Version/role | SHA-256 | Existing license family | Local modification/source consequence |
|---|---|---|---|---|
| `freebl3.dll` | NSS 3.42 Basic ECC Beta | `12808c651528c9e08f5ccf86af00f9061b19103c0c712d376755664f41ee474d` | MPL-2.0 | modified by the NT4 fail-closed RNG patch; exact post-patch source and patch must be offered |
| `freebl3.chk` | freebl integrity companion | `122aefebbfd76eb68352eb23d59d4caff70183b520f81b5017bf299f7b745daa` | follows originating NSS component | preserve with matching freebl binary/source |
| `nss3.dll` | NSS 3.42 Beta | `67634df027fc416127bd5d4d6762edf5be5dce5fa74eb006710c11621000d4a9` | MPL-2.0 | unmodified relative to recorded snapshot |
| `nssutil3.dll` | NSS 3.42 Beta | `b6c6221fcce382b363e4cf9e2ca8357f7c4f8daa10970bda06836df6576d62d6` | MPL-2.0 | unmodified relative to recorded snapshot |
| `ssl3.dll` | NSS 3.42 Beta | `a45adb3ca8abfab4716315acba515f0de1ad4c56b095c89f32042dfc120277da` | MPL-2.0 | unmodified relative to recorded snapshot |
| `softokn3.dll` | NSS 3.42 Basic ECC Beta | `ea59b260ac3f2e37ca63ce4aa50d470293c462d644accc27f3dbdb9a6b50ff70` | MPL-2.0 | unmodified relative to recorded snapshot |
| `softokn3.chk` | softokn integrity companion | `e6ddde563c1ed8ebfbd43b228714dd3134ff4f458e70fca148e2ca93de546584` | follows originating NSS component | preserve with matching softokn binary/source |
| `nspr4.dll` | NSPR 4.7.7 | `1ddd21185278db4f013295db61c751ac95fa15cf5007aa1de42599281a6f66a1` | MPL-1.1/GPL-2.0-or-later/LGPL-2.1-or-later tri-license | preserve upstream choices and notices; do not relicense as PST MPL-2.0 |
| `plc4.dll` | NSPR 4.7.7 | `b244252d7e931d961b723e18a817831931ca7b5be038bb5b8981bacd0deb684c` | MPL-1.1/GPL-2.0-or-later/LGPL-2.1-or-later tri-license | same |
| `plds4.dll` | NSPR 4.7.7 | `7a4278ed168f9b6d6a958c6c404058553747c6f084671e5fd20e90c68dee172d` | MPL-1.1/GPL-2.0-or-later/LGPL-2.1-or-later tri-license | same |

The only recorded local third-party patch is `third_party/retrozilla-nss/patches/0001-win32-secure-rng-fail-closed-nt4.patch`, which changes the MPL-2.0-covered `security/nss/lib/freebl/win_rand.c`. The legacy SDK must preserve `NSS-MPL-2.0.txt`, `RetroZilla-LICENSE.txt`, `RetroZilla-LEGAL.txt`, the NSPR tri-license evidence, provenance/manifests, and a clear route to the exact source snapshot and ordered patch.

The NSPR GPL/LGPL alternatives are optional license choices; shipping the NSPR files under their MPL-1.1 option does not force PST or its consumer application under GPL/LGPL. Those dependencies retain their existing license notices and are not absorbed into the PST MPL-2.0 designation.

## OpenSSL 3.5.8 boundary

OpenSSL 3.0 and later use Apache License 2.0. The prepared OpenSSL 3.5.8 component contains its Apache-2.0 `LICENSE`; no upstream `NOTICE` file was found in the prepared tree. The staged OpenSSL DLLs and import libraries remain Apache-2.0 components and are not relicensed as MPL-2.0.

MPL-2.0 and Apache-2.0 files can coexist in a Larger Work while retaining their respective terms. If the exact upstream OpenSSL release or later staging contains a `NOTICE`, it must also be preserved. The release notices must clearly separate PST, OpenSSL, NSS, and NSPR licensing.

## Windows and Microsoft boundary

Schannel, CryptoAPI, CNG, Winsock, and their system DLLs are supplied by Windows and are not copied into any candidate SDK. VC6, current Visual Studio Build Tools, Windows SDK files, NMAKE, and compiler libraries are build inputs and are not staged. The modern CRT/UCRT is also not staged by PST; any consumer deployment of Microsoft redistributables is a separate action under Microsoft's applicable terms.

No Microsoft DLL or import/static library was found accidentally staged as a PST redistributable. This separation must remain true in the final release manifest.

## Current third-party notices audit

`THIRD_PARTY_NOTICES.md` distinguishes PST from redistributed third-party components, identifies the RetroZilla NSS/NSPR lineage and OpenSSL 3.5.8, and does not present Windows system facilities or toolchains as redistributed dependencies. Its component versions and basic license references agree with the current staging inventory. It must still be finalized after owner approval to add the operative PST license declaration and an explicit, package-visible route to corresponding source. No factual correction to that file is required in this audit.

## Proprietary distributor consequences

For `ProdutoFechado.exe` statically linked with PST:

- the company's independently written application files may remain proprietary;
- the whole executable does not have to be relicensed under MPL-2.0;
- PST files included without modification remain MPL-covered, and recipients must receive the notices and a clear way to obtain their corresponding Source Code Form;
- modifications made to existing PST covered files must be made available in Source Code Form under MPL-2.0;
- new, separate application files are not automatically covered merely because they statically link PST;
- the distributor may impose additional terms on the Larger Work, provided those terms do not restrict recipients' MPL rights in Covered Software;
- redistributed NSS/NSPR and OpenSSL binaries retain their own independent notice and source obligations.

## Recommended release license layout

After approval, use a root `LICENSE` for PST's unmodified MPL 2.0 text, retain `THIRD_PARTY_NOTICES.md`, and place dependency-specific texts under the SDK's `licenses/` directory. Every SDK README or manifest should name the PST license, enumerate redistributed third parties, point to their license texts, and identify the exact corresponding-source package. Root and translated READMEs need only a concise future `License` link; they are not changed by this audit.

## Compatibility with downstream licenses

| Downstream model | Result | Conditions |
|---|---|---|
| Proprietary application | compatible | separate application files may remain proprietary; comply with MPL for PST covered files and modifications |
| MIT/BSD application or components | compatible | each file keeps its license and required notices |
| Apache-2.0 application or OpenSSL component | compatible | preserve Apache license, copyright and any NOTICE obligations separately |
| GPL/LGPL/AGPL Larger Work | conditionally compatible | use standard MPL-2.0 without Exhibit B and satisfy Section 3.3/Secondary License conditions for an actual Larger Work |

The MPL patent grant applies to contributor claims as described in Section 2.1(b), subject to the limitations and defensive termination provisions in the license. It does not grant rights for unrelated third-party components.

## Conditions before applying or releasing

1. Obtain the owner's explicit approval of MPL-2.0 and of the recommendation to use the standard text without Exhibit B.
2. Confirm copyright ownership or sufficient licensing authority for every intended project-authored file and contribution.
3. Add an authoritative root `LICENSE` containing the unmodified MPL 2.0 text and adopt a consistent Exhibit A or SPDX notice policy for project-owned files only.
4. Do not apply PST notices to `third_party/`, generated third-party binaries, or files carrying their own terms.
5. Publish corresponding source concurrently with every binary SDK. Include exact PST covered sources and build/modification scripts.
6. For the legacy SDK, include or unequivocally link the exact RetroZilla source snapshot, ordered patch, manifests, existing license texts, and legal notices. The modified `freebl` source must be available.
7. Preserve the OpenSSL Apache-2.0 license and any applicable copyright/NOTICE material.
8. Keep Microsoft system/toolchain components out of PST packages unless separately inventoried and reviewed under their redistribution terms.
9. Update `THIRD_PARTY_NOTICES.md`, SDK source-availability instructions, and release manifests so a recipient can identify every component and obtain corresponding source.
10. Re-run the final package/license inventory after the license is applied; Phase 9.F must not start while these gates remain open.

## Application plan after explicit approval

After the owner states `Aprovado, aplique MPL-2.0`, perform a dedicated, reviewable licensing change:

1. add the official unmodified MPL 2.0 text as root `LICENSE`;
2. add the selected short notice/SPDX form only to project-owned files;
3. update README files, package metadata, SDK README, notices, and manifests;
4. stage exact corresponding-source archives and verify their hashes against binaries;
5. inspect all four SDK candidates for license/source completeness;
6. run the normal VC6 and modern MSVC regressions plus clean staging validation;
7. record `git diff --check`, package manifests, hashes, and the release-license gate result.

Until that explicit approval and implementation, MPL-2.0 remains a technically validated candidate rather than the repository's operative license.

## Primary references consulted

- Mozilla Public License 2.0 text: https://www.mozilla.org/MPL/2.0/
- Mozilla MPL 2.0 FAQ: https://www.mozilla.org/MPL/2.0/FAQ/
- Mozilla MPL 2.0 revision FAQ: https://www.mozilla.org/MPL/2.0/Revision-FAQ/
- Mozilla guidance on combining MPL and GPL: https://www.mozilla.org/MPL/2.0/combining-mpl-and-gpl/
- Mozilla license policy: https://www.mozilla.org/MPL/license-policy/
- OpenSSL license page: https://openssl-library.org/source/license/
- OpenSSL source/release page: https://www.openssl-library.org/source/
- OpenSSL 3.5 introduction: https://docs.openssl.org/3.5/man7/ossl-guide-introduction/