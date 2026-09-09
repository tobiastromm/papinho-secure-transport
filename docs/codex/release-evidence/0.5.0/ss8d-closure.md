<!-- SPDX-License-Identifier: MPL-2.0 -->

# SS-8D documentation, examples and package closure

Status: PASS. Library/package 0.5.0, public API 2.0.0, provider SPI 3.0.

## Documentation and examples

- Canonical English and Brazilian Portuguese guides cover CLIENT/SERVER, explicit role, per-connection EXACT/ORDERED/AUTOMATIC selection, role masks, pinning/no post-binding fallback, identity/authentication/trust/name separation, ALPN, Peer Info, bounded progress, reciprocal shutdown, truncation and the application-owned listener boundary with semantic parity.
- API 2.0 and SPI 3.0 documents match the current headers. The migration guide separately covers consumer and provider-author migration.
- Provider documents record factual asymmetry: OpenSSL CLIENT/SERVER TLS 1.2/1.3 and SERVER ALPN; Schannel SERVER TLS 1.2 without validated TLS 1.3 or complete SERVER ALPN; NSS CLIENT/SERVER TLS 1.2/1.3 without SERVER SYSTEM_TRUST or complete SERVER ALPN.
- `examples/basic_server.c` demonstrates application bind/listen/accept, passes only an accepted connected socket to PST, and preserves listener ownership. All seven repository examples compile with MSVC `/W4 /WX`; the SERVER example also compiles with VC6 `/W4 /WX` using the target's `WIN32_LEAN_AND_MEAN` definition.
- Public Markdown audit: zero broken local links and zero personal absolute paths.

## Package hygiene and validation

- Four SS-8 transfer-only scripts were identified as unwanted source-package inputs and excluded by the source stager. `docs/codex`, build/dist/work trees, logs, screenshots, backups, dumps, objects and temporary outputs remain excluded.
- Final archive audit: source 544 entries; Schannel 33; OpenSSL 38; Combined 38; NSS 47; unwanted entries zero.
- SDKs ship both EN/PT guides, API/SPI/migration/provider documentation, security lifecycle material, examples, exact target manifest, static library and declared runtime dependencies.
- All five packages passed external hash, ZIP layout, clean extraction, complete internal SHA-256, archive, licensing and corresponding-source checks. Eight extracted CLIENT/SERVER consumers passed compile/link/runtime, and the extracted Combined public selection test passed.
- Packaged CLIENT and SERVER examples compiled and linked from the extracted final Schannel SDK with `/W4 /WX` and no checkout include/library input.
- Canonical output plus two sequential independent runs produced byte-identical ZIPs and checksum file.

## Final hashes

- Source: `4df9945b0a219c9a04bc9429faeeeb10fb449fcc945bfdcbd6b1fe605e53c673`
- NSS: `91d004e113c662386bc947658d0a11d7c3022066b647a8d361ba2c862de39a9f`
- Schannel: `6e9ed7467051c2946ebc146f8fec49724759ed102b9e733191f0345125ce8a63`
- OpenSSL: `625959d8fd73cd3a73ea0f6ee23761fb19a1b20899016e3cb74f5aa9388b6635`
- Combined: `19c43680e981a01cbd48a486fd12c53d5c50167d42b9f2cebf9955fa7c823707`
- `SHA256SUMS-packages.txt`: `900ba5d74d5c9c8a84a9fe7c2464891ef17000a0df3ded8384a2e34958f0cae1`

No production/API/SPI/library/runtime binary changed. The previous real clean-machine and NT4 TLS evidence therefore remains linked by binary identity; neither real-TLS rerun is required. No tag, release, asset upload or master mutation occurred.
