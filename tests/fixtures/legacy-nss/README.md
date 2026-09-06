<!-- SPDX-License-Identifier: MPL-2.0 -->

# Legacy NSS test evidence

No executable or credential fixture is stored here.

The original failure-injection DLL/CHK evidence is superseded because the CHK does not match that DLL, making its negative result causally ambiguous.

The canonical v3 A/B evidence also remains outside git. Its DLL and matching shlibsign CHK are TEST ONLY - FORCES SECURE RNG FAILURE WHEN `PAPACC_TEST_FORCE_SECURE_RNG_FAILURE=1` - DO NOT SHIP - DO NOT USE AS PRODUCTION BACKEND. Hashes, procedure, and results are recorded in `docs/legacy-nss-provenance.md`.

The baseline remains external. No private key, client DB, password, certificate DB, v3 DLL, or v3 CHK was copied.
