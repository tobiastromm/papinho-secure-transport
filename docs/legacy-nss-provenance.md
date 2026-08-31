# Legacy RetroZilla NSS provenance

## Inputs and source

Read-only archive: C:/PapinhoBuildArchive/RetroZilla-NSS-NT4-VC6-2026-08-30.

- mozilla-build.zip: 62,968,122 bytes; SHA-256 985f167215ef501f1c0cfad84c2e192628f30ac0c451503b75384ea9fecaf548.
- projects.zip: 187,341,470 bytes; SHA-256 166dc29fba897cdfe11faeca49559d9e9a5df778775c04fd74bd608b9f109866.

Audit extraction used C:/PSTW (the longer initial PowerShell extraction hit MAX_PATH and was retained separately). The ZIPs preserve environment files, not a bootable/reproducible VM; the original VM/snapshot is separate evidence.

The source layout is projects/RetroZilla/RetroZilla with client.mk, mozconfig, security/nss, and nsprpub; output is obj-rzSuite-release and logs are under projects/RetroZilla/logs. The archive lacks Git metadata. Commit 2f274574d3c6ee8769914046920d649bbae9f81b is therefore the external investigation provenance record, not independently derivable from the ZIP.

Source headers identify NSS 3.42 Beta and NSPR 4.7.7. Build environment: VC6 (_MSC_VER=1200), VS6 SP5, Processor Pack, MozillaBuild 1.2, Win32 x86. mozconfig targets i586-pc-msvc and uses /c/projects/RetroZilla/obj-rzSuite-release.

## Build reconstruction

Keep the source directory literally named RetroZilla. In MozillaBuild use /bin/make.exe 3.79.1, not /local/bin/make.exe 3.81.90 (which causes client.mk multiple-target-pattern errors):

    export PATH=/bin:$PATH
    hash -r
    export MAKE=/bin/make.exe

Run through client.mk so historical variables propagate. Logs show cd freebl; /bin/make.exe -j1 libs, FREEBL_CHILD_BUILD=1, the WIN95_SINGLE_SHLIB object directory, sysrand.obj linking into freebl3.dll with nssutil3.lib, nspr4.lib, and advapi32.lib, followed by shlibsign producing freebl3.chk. A later ChatZilla packaging failure for missing chatzilla.jar occurs after valid NSS/freebl output and is irrelevant to that artifact.

## Fail-closed selection

The definitive production candidate is projects/RetroZilla/nt4-failclosed-pair/freebl3.dll and freebl3.chk. It is byte-identical to the separately named failclosed pair and matches both historical MD5 values:

- DLL: MD5 619d2b57b0c0146425b267015dc88e57; SHA-256 12808c651528c9e08f5ccf86af00f9061b19103c0c712d376755664f41ee474d.
- CHK: MD5 6ec845b97a9ff585eaee00a44e95fe48; SHA-256 122aefebbfd76eb68352eb23d59d4caff70183b520f81b5017bf299f7b745daa.

The patch is patches/0001-win32-secure-rng-fail-closed-nt4.patch. It preserves SystemFunction036, CryptAcquireContextA, CryptGenRandom, and CryptoAPI; it removes environmental-noise fallback from RNG_SystemRNG, introduces no RNG, and contains no test macro.

Baseline remains external: DLL MD5 e8fe1e414d43075c13eb457c870c00c9 / SHA-256 6e108943c4535f6b758f8daf12922ab6deeb7526665b3bee958577896b581766; CHK MD5 2ebf5a1cd01b7e199198a1be71a2ebb / SHA-256 b3bf8d516492ea7a8e002c07e01beba9a2153233dd007ee6df0004df4711a0e5.

The original failure-injection evidence remains external and TEST ONLY / DO NOT SHIP: DLL SHA-256 0405889237676985b8201a5629ec82f46dfbe352b36d7fb06b4934789d93dda8. Its archived CHK has SHA-256 122aef... and belongs to the normal build. Phase 0.A discovered this DLL/CHK mismatch. Consequently, the old NSS_Init failure could have been caused either by injected entropy failure or by integrity-check failure. That negative result is retained as historical evidence but is **SUPERSEDED / AMBIGUOUS** and is not the canonical fail-closed proof.

## Phase 0.A-R1 controlled A/B revalidation

Canonical replacement evidence is stored read-only outside the repository under:

    C:/PapinhoBuildArchive/RetroZilla-NSS-NT4-VC6-2026-08-30/revalidation-2026-08-30/

The VC6 source was first restored to the normal fail-closed version. A TEST-ONLY v3 variant then conditioned failure on `PAPACC_TEST_FORCE_SECURE_RNG_FAILURE=1`: absent means normal fail-closed behavior; set to 1 makes `RNG_SystemRNG` return zero. VC6 rebuilt `sysrand.c` to `sysrand.obj` and relinked `freebl3.dll`.

The first manual shlibsign invocation lacked `plc4.dll`; this was an environment/PATH issue, not a v3 DLL defect. With `obj-rzSuite-release/dist/bin` prepended to PATH, shlibsign ran against the exact v3 DLL, reported the DLL and CHK paths, generated its hash/signature, and exited zero. Archive provenance records `cmp` exit zero for build-output versus preserved DLL and CHK.

External evidence hashes:

- `failure-injection-v3/freebl3.dll`: 380,928 bytes; MD5 `a84affc915dd5c816521f5ac40d4b454`; SHA-256 `2c98dc3e73b2dbe425213ad151c841c15febd3ecfdbef06f779f7018b0c184d3`.
- `failure-injection-v3/freebl3.chk`: 899 bytes; MD5 `c62826899147f4e8f8d9f56cc8b78862`; SHA-256 `09f0c3a73f1e1783e2640f857fa82f9c30b3c9c3c1b8511ac798c69cd18ece3d`.
- `logs/freebl-rng-failure-v3.log`: 846,302 bytes; SHA-256 `2a95cfd51c5b60673abfad2388ab64ccde513e5e7470cf657c89bd79175bba78`.

Test A used that same v3 DLL/CHK pair on NT4 with the variable absent. NSS_Init, TCP, nonblocking incremental handshake, TLS 1.3 mTLS, ALPN `papacc/1`, peer certificate, payload, partial-read reconstruction, and clean close all passed. The negotiated cipher was 0x1303, group 29, with no resumption or early data.

Test B changed no binary or dependency and set `PAPACC_TEST_FORCE_SECURE_RNG_FAILURE=1`. NSS_Init failed with `SEC_ERROR_PKCS11_DEVICE_ERROR` (-8023), before TCP success from the probe, TLS, ALPN, authentication, or payload. The server's `ACCEPT` line only denotes a listening socket; it is not a successful handshake.

Because the DLL, matching CHK, NT4 system, harness, server, and all other dependencies were identical, with only the environment-controlled injection changed, the A/B test removes the competing DLL/CHK-mismatch explanation. V3 is now the canonical negative evidence that unavailable secure entropy fails closed. It is **TEST-ONLY / DO NOT SHIP** and is not part of the runtime manifest.

After the experiment the working source was restored. `win_rand.c` and `win_rand.c.failclosed` both have SHA-256 `4dddc83482544c3757e81ffbfebe1fa96fcee0ff5205c00a0ce2a93d19b304d4`, and the production source contains no `PAPACC_TEST` injection.

## Validation and security audit

Separate NT4 SP6 evidence recorded TLS 1.3 (0x0304), ChaCha20-Poly1305 (0x1303), X25519 (group 29), ALPN papacc/1, mTLS, payload/partial reads, and clean close through CryptoAPI. The Phase 0.A-R1 controlled A/B experiment above supersedes the ambiguous original negative test.

The archive scan found no credential DB, .key/.p12/.pfx, password file, or private PEM block in the projects material. No secret, certificate DB, or test credential was copied. Only runtime DLL/CHK files, license texts, this patch, and documentation were ingested.
