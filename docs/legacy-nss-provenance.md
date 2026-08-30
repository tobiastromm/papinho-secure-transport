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

Failure injection remains external and TEST ONLY / DO NOT SHIP: DLL SHA-256 0405889237676985b8201a5629ec82f46dfbe352b36d7fb06b4934789d93dda8. Its archived CHK has SHA-256 122aef... and belongs to the normal build; the injection build log shows shlibsign failure, so it is deliberately not represented as a valid pair.

## Validation and security audit

Separate NT4 SP6 evidence recorded TLS 1.3 (0x0304), ChaCha20-Poly1305 (0x1303), X25519 (group 29), ALPN papacc/1, mTLS, payload/partial reads, and clean close through CryptoAPI. Forced RNG failure made NSS_Init fail with SEC_ERROR_PKCS11_DEVICE_ERROR (-8023) before TLS activity.

The archive scan found no credential DB, .key/.p12/.pfx, password file, or private PEM block in the projects material. No secret, certificate DB, or test credential was copied. Only runtime DLL/CHK files, license texts, this patch, and documentation were ingested.
